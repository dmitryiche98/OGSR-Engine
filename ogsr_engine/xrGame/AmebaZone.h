#pragma once

class CAmebaZone : public CVisualZone, public CPHUpdateObject
{
    typedef CVisualZone inherited;
    float m_fVelocityLimit;
    u32 m_dwHitPeriod; // ms between hits while blowing out (hit_period; 0 = every update, as in SoC)
    u32 m_dwLastHitTime;

public:
    CAmebaZone();
    ~CAmebaZone();
    virtual void Affect(SZoneObjectInfo* O);

protected:
    virtual void PhTune(dReal step);
    virtual void PhDataUpdate(dReal step) { ; }
    virtual bool BlowoutState();
    virtual void SwitchZoneState(EZoneState new_state);
    virtual void Load(LPCSTR section);
    virtual float distance_to_center(CObject* O);
};
