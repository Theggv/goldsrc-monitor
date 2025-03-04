#include "displaymode_entityreport.h"
#include "cvars.h"
#include "utils.h"
#include "client_module.h"
#include "engine_module.h"
#include "studio.h"
#include "entity_dictionary.h"
#include "local_player.h"
#include "bounding_box.h"
#include <algorithm>
#include <iterator> 

void CModeEntityReport::Render2D(int scrWidth, int scrHeight, CStringStack &screenText)
{
    vec3_t entityOrigin;
    vec3_t entityAngles;
    CBoundingBox entityBbox;

    if (!g_EntityDictionary.IsInitialized())
        g_EntityDictionary.Initialize();

    screenText.Clear();
    m_iEntityIndex = TraceEntity();
    if (!m_iEntityIndex)
    {
        std::string mapName = Utils::GetCurrentMapName();
        screenText.Push("Entity not found");
        screenText.PushPrintf("Map: %s", mapName.c_str());
        screenText.PushPrintf("Entity descriptions: %d", g_EntityDictionary.GetDescriptionsCount());
    }
    else if (Utils::IsGameDirEquals("cstrike") && g_LocalPlayer.IsSpectate() && g_pPlayerMove->iuser3 != 3)
    {
        // disable print in non free-look spectating modes
        screenText.Push("Print enabled only in free look mode");
        m_iEntityIndex = 0;
    }
    else
    {
        CEntityDescription entityDesc;
        cl_entity_t *entity = g_pClientEngfuncs->GetEntityByIndex(m_iEntityIndex);
        vec3_t entityVelocity = Utils::GetEntityVelocityApprox(m_iEntityIndex);
        bool isDescFound = g_EntityDictionary.FindDescription(m_iEntityIndex, entityDesc);
        const vec3_t centerOffset = (entity->curstate.mins + entity->curstate.maxs) / 2.f;

        entityAngles = entity->curstate.angles;
        entityOrigin = entity->origin + centerOffset;
        Utils::GetEntityBoundingBox(m_iEntityIndex, entityBbox);

        screenText.PushPrintf("Entity Index: %d", m_iEntityIndex);
        screenText.PushPrintf("Origin: (%.1f; %.1f; %.1f)",
            entityOrigin.x, entityOrigin.y, entityOrigin.z);
        screenText.PushPrintf("Distance: %.1f units",
            GetEntityDistance(m_iEntityIndex));
        screenText.PushPrintf("Velocity: %.2f u/s (%.1f; %.1f; %.1f)", 
            entityVelocity.Length2D(), entityVelocity.x, entityVelocity.y, entityVelocity.z);
        screenText.PushPrintf("Angles: (%.1f; %.1f; %.1f)",
            entityAngles.x, entityAngles.y, entityAngles.z);
        screenText.PushPrintf("Hull Size: (%.1f; %.1f; %.1f)",
            entityBbox.GetSize().x, entityBbox.GetSize().y, entityBbox.GetSize().z);
        screenText.PushPrintf("Movetype: %s", Utils::GetMovetypeName(entity->curstate.movetype));
        screenText.PushPrintf("Render Mode: %s", Utils::GetRenderModeName(entity->curstate.rendermode));
        screenText.PushPrintf("Render FX: %s", Utils::GetRenderFxName(entity->curstate.renderfx));
        screenText.PushPrintf("Render Amount: %d", entity->curstate.renderamt);
        screenText.PushPrintf("Render Color: %d %d %d", 
            entity->curstate.rendercolor.r, 
            entity->curstate.rendercolor.g, 
            entity->curstate.rendercolor.b
        );

        if (isDescFound)
        {
            const std::string &classname = entityDesc.GetClassname();
            const std::string &targetname = entityDesc.GetTargetname();
            screenText.PushPrintf("Classname: %s", classname.c_str()); 
            if (targetname.length() > 0)
                screenText.PushPrintf("Targetname: %s", targetname.c_str()); 
        }

        if (entity->model->type == mod_studio)
        {
            std::string modelName;
            Utils::GetEntityModelName(m_iEntityIndex, modelName);
            screenText.PushPrintf("Model Name: %s", modelName.c_str());
            screenText.PushPrintf("Anim. Frame: %.1f", entity->curstate.frame);
            screenText.PushPrintf("Anim. Sequence: %d", entity->curstate.sequence);
            screenText.PushPrintf("Bodygroup Number: %d", entity->curstate.body);
            screenText.PushPrintf("Skin Number: %d", entity->curstate.skin);
        }

        if (isDescFound)
        {
            const int propsCount = entityDesc.GetPropertiesCount();
            if (propsCount > 0)
            {
                std::string propsString;
                screenText.Push("Entity Properties");
                for (int i = 0; i < propsCount; ++i)
                {
                    entityDesc.GetPropertyString(i, propsString);
                    screenText.PushPrintf("    %s", propsString.c_str());
                }
            }
            else {
                screenText.Push("No entity properties");
            }
        }
        else
            screenText.Push("Entity properties not found");
    }

    Utils::DrawStringStack(
        static_cast<int>(ConVars::gsm_margin_right->value), 
        static_cast<int>(ConVars::gsm_margin_up->value), 
        screenText
    );
}

void CModeEntityReport::Render3D()
{
    cl_entity_t *entity;
    CBoundingBox entityBbox;
    const Color colorGreen = Color(0.f, 1.f, 0.f, 1.f);

    if (m_iEntityIndex <= 0) 
        return;

    if (!g_EngineModule.IsSoftwareRenderer())
    {
        entity = g_pClientEngfuncs->GetEntityByIndex(m_iEntityIndex);
        Utils::GetEntityBoundingBox(m_iEntityIndex, entityBbox);
        vec3_t centerOffset = (entity->curstate.mins + entity->curstate.maxs) / 2.f;
        Utils::DrawCuboid(entity->origin, centerOffset, entity->angles, entityBbox.GetSize(), colorGreen);
    }
}

void CModeEntityReport::HandleChangelevel()
{
    g_EntityDictionary.Reset();
}

int CModeEntityReport::TraceEntity()
{
    vec3_t viewDir;
    vec3_t viewOrigin;
    pmtrace_t traceData;
    const float lineLen = 11590.0f;
    float worldDistance = lineLen;
    int ignoredEnt = -1;

    m_EntityIndexList.clear();
    m_EntityDistanceList.clear();
    viewOrigin = g_LocalPlayer.GetViewOrigin();
    viewDir = g_LocalPlayer.GetViewDirection();

    if (g_LocalPlayer.IsSpectate())
        ignoredEnt = g_LocalPlayer.GetSpectateTargetIndex();  

    Utils::TraceLine(viewOrigin, viewDir, lineLen, &traceData, ignoredEnt);
    if (traceData.fraction < 1.f)
    {
        if (traceData.ent > 0)
            return g_pClientEngfuncs->pEventAPI->EV_IndexFromTrace(&traceData);
        else
            worldDistance = lineLen * traceData.fraction;
    }

    const int listCount = 3;
    physent_t *physEntLists[listCount] = { g_pPlayerMove->visents, g_pPlayerMove->physents, g_pPlayerMove->moveents };
    int physEntListsLen[listCount] = { g_pPlayerMove->numvisent, g_pPlayerMove->numphysent, g_pPlayerMove->nummoveent };
    for (int i = 0; i < listCount; ++i)
    {
        int physEntIndex = TracePhysEntList(physEntLists[i], physEntListsLen[i], viewOrigin, viewDir, lineLen);
        if (physEntIndex)
        {
            m_EntityIndexList.push_back(physEntIndex);
            m_EntityDistanceList.push_back(GetEntityDistance(physEntIndex));
        }
    }

    // get nearest entity from all lists
    // also add world for comparision
    m_EntityIndexList.push_back(0);
    m_EntityDistanceList.push_back(worldDistance);
    auto &distanceList = m_EntityDistanceList;
    if (distanceList.size() > 1)
    {
        auto iterNearestEnt = std::min_element(std::begin(distanceList), std::end(distanceList));
        if (std::end(distanceList) != iterNearestEnt)
            return m_EntityIndexList[std::distance(distanceList.begin(), iterNearestEnt)];
    }
    return 0;
}

float CModeEntityReport::TracePhysEnt(const physent_t &physEnt, vec3_t &viewOrigin, vec3_t &viewDir, float lineLen)
{
    CBoundingBox entityBbox;
    Utils::GetEntityBoundingBox(physEnt.info, entityBbox);

    // skip studiomodel visents which is culled
    vec3_t bboxMins = entityBbox.GetMins();
    vec3_t bboxMaxs = entityBbox.GetMaxs();
    if (!g_pClientEngfuncs->pTriAPI->BoxInPVS(bboxMins, bboxMaxs)) {
        return 1.0f;
    }

    // check for intersection
    vec3_t lineEnd = viewOrigin + (viewDir * lineLen);
    return Utils::TraceBBoxLine(entityBbox, viewOrigin, lineEnd);
}

int CModeEntityReport::TracePhysEntList(physent_t list[], int count, vec3_t &viewOrigin, vec3_t &viewDir, float lineLen)
{
    int entIndex = 0;
    float minFraction = 1.0f;
    
    for (int i = 0; i < count; ++i)
    {
        const physent_t &visEnt = list[i];
        float traceFraction = TracePhysEnt(visEnt, viewOrigin, viewDir, lineLen);
        if (traceFraction < minFraction)
        {
            entIndex    = visEnt.info;
            minFraction = traceFraction;
        }
    }

    return entIndex;
}

float CModeEntityReport::GetEntityDistance(int entityIndex)
{
    vec3_t pointInBbox;
    CBoundingBox entityBbox;
    cl_entity_t *entity = g_pClientEngfuncs->GetEntityByIndex(entityIndex);
    if (entity)
    {
        model_t *entityModel = entity->model;
        vec3_t viewOrigin = g_LocalPlayer.GetViewOrigin();

        // get nearest bbox-to-player distance by point caged in bbox
        Utils::GetEntityBoundingBox(entityIndex, entityBbox);
        const vec3_t &bboxMins = entityBbox.GetMins();
        const vec3_t &bboxMaxs = entityBbox.GetMaxs();
        pointInBbox.x = max(min(viewOrigin.x, bboxMaxs.x), bboxMins.x);
        pointInBbox.y = max(min(viewOrigin.y, bboxMaxs.y), bboxMins.y);
        pointInBbox.z = max(min(viewOrigin.z, bboxMaxs.z), bboxMins.z);
        return (pointInBbox - viewOrigin).Length();
    }
    return 0.0f;
}
