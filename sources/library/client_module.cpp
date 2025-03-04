#include "client_module.h"
#include "engine_module.h"
#include "build_info.h"
#include "exception.h"
#include "utils.h"

cl_enginefunc_t *g_pClientEngfuncs;
CClientModule& g_ClientModule = CClientModule::GetInstance();

CClientModule& CClientModule::GetInstance()
{
    static CClientModule instance;
    return instance;
}

bool CClientModule::FindHandle()
{
    HANDLE currProcess = GetCurrentProcess();
    m_hModule = Utils::FindModuleByExport(currProcess, "V_CalcRefdef");
    Utils::GetModuleInfo(currProcess, m_hModule, m_ModuleInfo);
    return m_hModule != NULL;
}

bool CClientModule::FindEngfuncs(const CBuildInfo &buildInfo)
{
    uint8_t *probeAddr;
    uint8_t *coincidenceAddr;
    uint8_t *scanStartAddr;
    uint8_t *moduleEndAddr;
    uint8_t *moduleAddr;
    uint8_t *pfnSPR_Load;
    uint8_t *pfnSPR_Frames;
    const CBuildInfoEntry &buildInfoEntry = buildInfo.GetInfoEntry();

    moduleAddr = g_EngineModule.GetAddress();
    scanStartAddr = moduleAddr;
    moduleEndAddr = moduleAddr + g_EngineModule.GetSize();
    
    // obtain address directly without searching
    if (buildInfoEntry.HasClientEngfuncsOffset()) {
        g_pClientEngfuncs = (cl_enginefunc_t *)(moduleAddr + buildInfoEntry.GetClientEngfuncsOffset());
        return true;
    }

    if (!g_EngineModule.GetFunctionsFromAPI(&pfnSPR_Load, &pfnSPR_Frames))
    {
        pfnSPR_Load = static_cast<uint8_t *>(buildInfo.FindFunctionAddress(
            FUNCTYPE_SPR_LOAD, moduleAddr, moduleEndAddr
            ));
        if (!pfnSPR_Load) {
            EXCEPT("SPR_Load() address not found");
        }

        pfnSPR_Frames = static_cast<uint8_t *>(buildInfo.FindFunctionAddress(
            FUNCTYPE_SPR_FRAMES, moduleAddr, moduleEndAddr
            ));
        if (!pfnSPR_Frames) {
            EXCEPT("SPR_Frames() address not found");
        }
    }

    while (true)
    {
        coincidenceAddr = (uint8_t *)Utils::FindMemoryInt32(
            scanStartAddr,
            moduleEndAddr,
            (uint32_t)pfnSPR_Load
        );
        if (!coincidenceAddr || scanStartAddr >= moduleEndAddr)
            EXCEPT("valid reference to SPR_Load() not found");
        else
            scanStartAddr = coincidenceAddr + sizeof(uint32_t);

        // check for module range to avoid segfault
        probeAddr = *(uint8_t **)(coincidenceAddr + sizeof(uint32_t));
        if (probeAddr >= moduleAddr && probeAddr < moduleEndAddr)
        {
            if (probeAddr == pfnSPR_Frames) 
            {
                g_pClientEngfuncs = (cl_enginefunc_t *)coincidenceAddr;
                return true; 
            }
        }
    }
    return false;
}

uint8_t* CClientModule::GetFuncAddress(const char *funcName)
{
    return reinterpret_cast<uint8_t*>(GetProcAddress(GetHandle(), funcName));
}
