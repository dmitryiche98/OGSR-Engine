#include "StdAfx.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "../Level.h"
#include "../ai_space.h"
#include "../script_engine.h"
#include "../PhysicsShellHolder.h"
#include "clsid_game.h"
#include "script_game_object.h"

struct SLuaWpnParams
{
    luabind::functor<float> m_functorRPM;
    luabind::functor<float> m_functorAccuracy;
    luabind::functor<float> m_functorDamage;
    luabind::functor<float> m_functorHandling;
    SLuaWpnParams();
    ~SLuaWpnParams();
};

SLuaWpnParams::SLuaWpnParams()
{
    bool functor_exists;
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetRPM", m_functorRPM);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetDamage", m_functorDamage);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetHandling", m_functorHandling);
    VERIFY(functor_exists);
    functor_exists = ai().script_engine().functor("ui_wpn_params.GetAccuracy", m_functorAccuracy);
    VERIFY(functor_exists);
}

SLuaWpnParams::~SLuaWpnParams() {}

SLuaWpnParams* g_lua_wpn_params = NULL;

void destroy_lua_wpn_params()
{
    if (g_lua_wpn_params)
        xr_delete(g_lua_wpn_params);
}

CUIWpnParams::CUIWpnParams()
{
    AttachChild(&m_progressAccuracyCmp);
    AttachChild(&m_progressDamageCmp);
    AttachChild(&m_progressHandlingCmp);
    AttachChild(&m_progressRPMCmp);

    AttachChild(&m_textAccuracy);
    AttachChild(&m_textDamage);
    AttachChild(&m_textHandling);
    AttachChild(&m_textRPM);

    AttachChild(&m_progressAccuracy);
    AttachChild(&m_progressDamage);
    AttachChild(&m_progressHandling);
    AttachChild(&m_progressRPM);
}

CUIWpnParams::~CUIWpnParams() {}

void CUIWpnParams::InitFromXml(CUIXml& xml_doc)
{
    if (!xml_doc.NavigateToNode("wpn_params", 0))
        return;

    CUIXmlInit::InitWindow(xml_doc, "wpn_params", 0, this);

    CUIXmlInit::InitStatic(xml_doc, "wpn_params:cap_accuracy", 0, &m_textAccuracy);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:cap_damage", 0, &m_textDamage);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:cap_handling", 0, &m_textHandling);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:cap_rpm", 0, &m_textRPM);

    CUIXmlInit::InitProgressBar(xml_doc, "wpn_params:progress_accuracy", 0, &m_progressAccuracy);
    CUIXmlInit::InitProgressBar(xml_doc, "wpn_params:progress_damage", 0, &m_progressDamage);
    CUIXmlInit::InitProgressBar(xml_doc, "wpn_params:progress_handling", 0, &m_progressHandling);
    CUIXmlInit::InitProgressBar(xml_doc, "wpn_params:progress_rpm", 0, &m_progressRPM);

    m_progressAccuracy.SetRange(0, 100);
    m_progressDamage.SetRange(0, 100);
    m_progressHandling.SetRange(0, 100);
    m_progressRPM.SetRange(0, 100);

    // Zone: optional comparison bars (what the actor wears), e.g. drawn under the main bars
    m_hasCmp = !!xml_doc.NavigateToNode("wpn_params:progress_damage_cmp", 0);
    CUIProgressBar* cmp_bars[] = {&m_progressAccuracyCmp, &m_progressDamageCmp, &m_progressHandlingCmp, &m_progressRPMCmp};
    const char* cmp_nodes[] = {"wpn_params:progress_accuracy_cmp", "wpn_params:progress_damage_cmp", "wpn_params:progress_handling_cmp", "wpn_params:progress_rpm_cmp"};
    for (int i = 0; i < 4; ++i)
    {
        if (m_hasCmp && xml_doc.NavigateToNode(cmp_nodes[i], 0))
        {
            CUIXmlInit::InitProgressBar(xml_doc, cmp_nodes[i], 0, cmp_bars[i]);
            cmp_bars[i]->SetRange(0, 100);
        }
        cmp_bars[i]->Show(false);
    }
}

void CUIWpnParams::SetInfo(CPhysicsShellHolder& obj, CPhysicsShellHolder* cmp)
{
    if (!g_lua_wpn_params)
        g_lua_wpn_params = xr_new<SLuaWpnParams>();

    const shared_str& wpn_section = obj.cNameSect();

    m_progressRPM.SetProgressPos(g_lua_wpn_params->m_functorRPM(*wpn_section, obj.lua_game_object()));
    m_progressAccuracy.SetProgressPos(g_lua_wpn_params->m_functorAccuracy(*wpn_section, obj.lua_game_object()));
    m_progressDamage.SetProgressPos(g_lua_wpn_params->m_functorDamage(*wpn_section, obj.lua_game_object()));
    m_progressHandling.SetProgressPos(g_lua_wpn_params->m_functorHandling(*wpn_section, obj.lua_game_object()));

    const bool show_cmp = m_hasCmp && cmp;
    m_progressAccuracyCmp.Show(show_cmp);
    m_progressDamageCmp.Show(show_cmp);
    m_progressHandlingCmp.Show(show_cmp);
    m_progressRPMCmp.Show(show_cmp);
    if (show_cmp)
    {
        const shared_str& cmp_section = cmp->cNameSect();
        m_progressRPMCmp.SetProgressPos(g_lua_wpn_params->m_functorRPM(*cmp_section, cmp->lua_game_object()));
        m_progressAccuracyCmp.SetProgressPos(g_lua_wpn_params->m_functorAccuracy(*cmp_section, cmp->lua_game_object()));
        m_progressDamageCmp.SetProgressPos(g_lua_wpn_params->m_functorDamage(*cmp_section, cmp->lua_game_object()));
        m_progressHandlingCmp.SetProgressPos(g_lua_wpn_params->m_functorHandling(*cmp_section, cmp->lua_game_object()));
    }
}

bool CUIWpnParams::Check(CPhysicsShellHolder& obj /*const shared_str& wpn_section*/)
{
    if (!READ_IF_EXISTS(pSettings, r_bool, obj.cNameSect(), "show_wpn_properties", true)) // allow to suppress default wpn params
    {
        return false;
    }

    if (pSettings->line_exist(obj.cNameSect(), "fire_dispersion_base"))
    {
        /*
        if (0==xr_strcmp(wpn_section, "wpn_addon_silencer"))
            return false;
        if (0==xr_strcmp(wpn_section, "wpn_binoc"))
            return false;
        if (0==xr_strcmp(wpn_section, "mp_wpn_binoc"))
            return false;*/

        if (obj.CLS_ID == CLSID_OBJECT_W_BINOCULAR || obj.CLS_ID == CLSID_OBJECT_W_KNIFE || obj.CLS_ID == CLSID_OBJECT_W_SILENCER || obj.CLS_ID == CLSID_OBJECT_W_SCOPE ||
            obj.CLS_ID == CLSID_OBJECT_W_GLAUNCHER)
            return false;

        return true;
    }
    else
        return false;
}