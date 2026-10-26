#include "stdafx.h"
#include "UIActorStats.h"
#include "UIXmlInit.h"
#include "UIScrollView.h"
#include "UIStatic.h"
#include "../UICursor.h"
#include "../ui_base.h"
#include "../string_table.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"

bool CUIActorStats::Init(CUIXml& xml, CUIWindow* owner)
{
    m_owner = owner;
    if (!xml.NavigateToNode("actor_stats", 0))
        return false;

    CUIXmlInit xml_init;
    m_list = xr_new<CUIScrollView>();
    m_list->SetAutoDelete(true);
    xml_init.InitScrollView(xml, "actor_stats", 0, m_list);
    m_list->SetWindowName("actor_stats");
    owner->AttachChild(m_list);
    m_period = xml.ReadAttribInt("actor_stats", 0, "period", 250);
    m_delay = xml.ReadAttribInt("actor_stats", 0, "delay", 300);

    if (xml.NavigateToNode("actor_stats_hint", 0))
    {
        if (xml.NavigateToNode("actor_stats_hint:frame", 0))
        {
            m_hint_frame = xr_new<CUIFrameWindow>();
            m_hint_frame->SetAutoDelete(true);
            xml_init.InitFrameWindow(xml, "actor_stats_hint:frame", 0, m_hint_frame);
            m_hint_frame->Show(false);
            owner->AttachChild(m_hint_frame);
        }
        m_hint = xr_new<CUIStatic>();
        m_hint->SetAutoDelete(true);
        xml_init.InitStatic(xml, "actor_stats_hint", 0, m_hint);
        m_hint->SetTextComplexMode(true);
        m_hint_pad = xml.ReadAttribFlt("actor_stats_hint", 0, "pad", 6.0f);
        m_hint->Show(false);
        owner->AttachChild(m_hint);
    }
    return true;
}

void CUIActorStats::Fill()
{
    if (!pSettings->line_exist("engine_callbacks", "ui_actor_stats_callback"))
        return;
    const int scroll = m_list->GetCurrentScrollPos();
    m_list->Clear(false);
    const char* callback = pSettings->r_string("engine_callbacks", "ui_actor_stats_callback");
    if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
        lua_function(m_list);
    m_list->SetScrollPos(scroll);
}

void CUIActorStats::Reset()
{
    m_next_fill = 0;
    m_hovered = nullptr;
    m_hovered_name = nullptr;
    HideHint();
}

void CUIActorStats::HideHint()
{
    m_hint_shown = false;
    if (m_hint)
        m_hint->Show(false);
    if (m_hint_frame)
        m_hint_frame->Show(false);
}

CUIWindow* CUIActorStats::FindHovered(const Fvector2& pos)
{
    Frect r;
    m_list->GetAbsoluteRect(r);
    if (!r.in(const_cast<Fvector2&>(pos)))
        return nullptr;
    for (u32 i = 0, n = m_list->GetSize(); i < n; ++i)
    {
        CUIWindow* w = m_list->GetItem(i);
        // own name only: WindowName() makes up "parent.child_N" for unnamed windows
        if (!w || !w->IsShown() || !w->WindowName_script() || !*w->WindowName_script())
            continue;
        Frect wr;
        w->GetAbsoluteRect(wr);
        if (wr.in(const_cast<Fvector2&>(pos)))
            return w;
    }
    return nullptr;
}

void CUIActorStats::ShowHint(const Fvector2& pos)
{
    m_hint->SetText(CStringTable().translate(m_hovered_name).c_str());
    const float w = m_hint->GetWidth();
    m_hint->AdjustHeightToText();
    m_hint->SetWidth(w);

    // the frame is the text plus `pad` on each side
    Frect r;
    r.set(0.0f, 0.0f, w + 2 * m_hint_pad, m_hint->GetHeight() + 2 * m_hint_pad);
    r.add(pos.x + 16.0f, pos.y + 16.0f);
    if (r.x2 > UI_BASE_WIDTH)
        r.sub(r.width() + 24.0f, 0.0f);
    if (r.y2 > UI_BASE_HEIGHT)
        r.sub(0.0f, r.y2 - UI_BASE_HEIGHT);
    if (r.x1 < 0.0f)
        r.add(-r.x1, 0.0f);
    if (r.y1 < 0.0f)
        r.add(0.0f, -r.y1);

    Frect owner_rect;
    m_owner->GetAbsoluteRect(owner_rect);
    if (m_hint_frame)
    {
        m_hint_frame->SetWndPos(r.x1 - owner_rect.x1, r.y1 - owner_rect.y1);
        m_hint_frame->SetWidth(r.width());
        m_hint_frame->SetHeight(r.height());
        m_hint_frame->Show(true);
    }
    m_hint->SetWndPos(r.x1 - owner_rect.x1 + m_hint_pad, r.y1 - owner_rect.y1 + m_hint_pad);
    m_hint->Show(true);
    m_hint_shown = true;
}

void CUIActorStats::Update()
{
    if (!m_list)
        return;

    if (Device.dwTimeGlobal >= m_next_fill)
    {
        m_next_fill = Device.dwTimeGlobal + m_period;
        Fill();
        m_hovered = nullptr; // rows may have been re-created
    }

    if (!m_hint)
        return;
    if (!m_owner->IsShown() || CUIDragDropListEx::m_drag_item)
    {
        m_hovered_name = nullptr;
        HideHint();
        return;
    }

    const Fvector2 pos = GetUICursor()->GetCursorPosition();
    CUIWindow* w = FindHovered(pos);
    const shared_str name = w ? shared_str(w->WindowName_script()) : shared_str{};
    m_hovered = w;
    if (name != m_hovered_name)
    {
        m_hovered_name = name;
        m_hover_start = Device.dwTimeGlobal;
        HideHint();
    }
    if (!w)
        return;
    if (m_hint_shown || Device.dwTimeGlobal >= m_hover_start + m_delay)
        ShowHint(pos);
}
