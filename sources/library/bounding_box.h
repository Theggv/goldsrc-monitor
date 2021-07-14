#pragma once
#include "hlsdk.h"

class CBoundingBox
{
public:
    CBoundingBox() {};
    CBoundingBox(const vec3_t &size);
    CBoundingBox(const vec3_t &vecMins, const vec3_t &vecMaxs);

    float GetSurfaceArea() const;
    vec3_t GetCenterPoint() const;
    void SetCenterToPoint(const vec3_t &point);
    inline const vec3_t &GetSize() const { return m_vecSize; };
    inline const vec3_t &GetMins() const { return m_vecMins; };
    inline const vec3_t &GetMaxs() const { return m_vecMaxs; };

private:
    void Initialize(const vec3_t &vecMins, const vec3_t &vecMaxs);

    vec3_t m_vecMins = vec3_t(0, 0, 0);
    vec3_t m_vecMaxs = vec3_t(0, 0, 0);
    vec3_t m_vecSize = vec3_t(0, 0, 0);
};
