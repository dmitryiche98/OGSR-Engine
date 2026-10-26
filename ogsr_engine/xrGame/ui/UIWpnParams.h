#pragma once
#include "UIWindow.h"

#include "UIProgressBar.h"

class CUIXml;

#include "../script_export_space.h"

struct SLuaWpnParams;
class CPhysicsShellHolder;

class CUIWpnParams : public CUIWindow
{
public:
    CUIWpnParams();
    virtual ~CUIWpnParams();

    void InitFromXml(CUIXml& xml_doc);
    // cmp (Zone): weapon to compare with - shown by the optional *_cmp bars (behind the main ones)
    void SetInfo(CPhysicsShellHolder& obj, CPhysicsShellHolder* cmp = nullptr);
    bool Check(CPhysicsShellHolder& obj /*const shared_str& wpn_section*/);

protected:
    // Zone: comparison bars, attached first so the main bars are drawn over them
    CUIProgressBar m_progressAccuracyCmp;
    CUIProgressBar m_progressHandlingCmp;
    CUIProgressBar m_progressDamageCmp;
    CUIProgressBar m_progressRPMCmp;
    bool m_hasCmp{};

    CUIProgressBar m_progressAccuracy;
    CUIProgressBar m_progressHandling;
    CUIProgressBar m_progressDamage;
    CUIProgressBar m_progressRPM;

    CUIStatic m_textAccuracy;
    CUIStatic m_textHandling;
    CUIStatic m_textDamage;
    CUIStatic m_textRPM;
};