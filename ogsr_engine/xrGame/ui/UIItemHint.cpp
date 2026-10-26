#include "stdafx.h"
#include "UIItemHint.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "../UICursor.h"
#include "../ui_base.h"
#include "../actor.h"
#include "../inventory.h"
#include "../Inventory_Item.h"
#include "../xr_3da/xr_input.h"

namespace
{
struct SHintSettings
{
    CUIItemHint::EMode mode{CUIItemHint::eModePanel};
    shared_str xml{"inventory_item_hint.xml"};
    u32 delay{300};
    bool loaded{};
};

SHintSettings& Settings()
{
    static SHintSettings s;
    if (!s.loaded)
    {
        s.loaded = true;
        constexpr const char* sect = "ui_item_hint";
        if (pSettings->section_exist(sect))
        {
            if (pSettings->line_exist(sect, "mode"))
            {
                const char* m = pSettings->r_string(sect, "mode");
                if (!xr_strcmp(m, "popup"))
                    s.mode = CUIItemHint::eModePopup;
                else if (!xr_strcmp(m, "both"))
                    s.mode = CUIItemHint::eModeBoth;
            }
            s.xml = READ_IF_EXISTS(pSettings, r_string, sect, "xml", "inventory_item_hint.xml");
            s.delay = READ_IF_EXISTS(pSettings, r_u32, sect, "delay", 300);
        }
    }
    return s;
}

bool HasSlot(CInventoryItem* item, u8 slot)
{
    for (const u8 s : item->GetSlots())
        if (s == slot)
            return true;
    return false;
}
} // namespace

CUIItemHint::EMode CUIItemHint::Mode() { return Settings().mode; }

CInventoryItem* CUIItemHint::CompareItem(CInventoryItem* item)
{
    CActor* actor = Actor();
    if (!actor || !item)
        return nullptr;
    const CInventory& inv = actor->inventory();

    CInventoryItem* res = nullptr;
    if (HasSlot(item, OUTFIT_SLOT))
        res = inv.ItemFromSlot(OUTFIT_SLOT);
    else if (HasSlot(item, HELMET_SLOT))
        res = inv.ItemFromSlot(HELMET_SLOT);
    else if (HasSlot(item, KNIFE_SLOT))
        res = inv.ItemFromSlot(KNIFE_SLOT);
    else if (HasSlot(item, FIRST_WEAPON_SLOT) || HasSlot(item, SECOND_WEAPON_SLOT))
    {
        // both weapon slots are equal: compare with the first one; the weapon in the first slot
        // (or anything, when the first slot is empty) - with the second one
        res = inv.ItemFromSlot(FIRST_WEAPON_SLOT);
        if (!res || res == item)
            res = inv.ItemFromSlot(SECOND_WEAPON_SLOT);
    }
    return res == item ? nullptr : res;
}

void CUIItemHint::Init(CUIWindow* owner, CUSTOMIZE customize)
{
    m_owner = owner;
    m_customize = std::move(customize);
    if (!PopupEnabled())
        return;

    m_info = xr_new<CUIItemInfo>();
    m_info->SetAutoDelete(true);
    owner->AttachChild(m_info);
    m_info->Init(Settings().xml.c_str());
    m_info->Show(false);
    // the item_info script keeps separate description lines for the popup and the static panel
    if (m_info->UIDesc)
        m_info->UIDesc->SetWindowName("item_hint_descr");
}

void CUIItemHint::Reset()
{
    m_hovered = nullptr;
    m_suppressed = nullptr;
    m_hover_start = 0;
    HidePopup();
}

void CUIItemHint::HidePopup()
{
    m_shown = nullptr;
    if (m_info && m_info->IsShown())
    {
        m_info->Show(false);
        m_info->InitItem(nullptr);
    }
}

CUICellItem* CUIItemHint::FindHovered(CUIWindow* w, const Fvector2& pos)
{
    for (CUIWindow* child : w->GetChildWndList())
    {
        if (child == m_info || !child->IsShown())
            continue;

        if (CUIDragDropListEx* list = dynamic_cast<CUIDragDropListEx*>(child))
        {
            Frect r;
            list->GetAbsoluteRect(r);
            if (!r.in(const_cast<Fvector2&>(pos)))
                continue;
            for (u32 i = 0, n = list->ItemsCount(); i < n; ++i)
            {
                CUICellItem* cell = list->GetItemIdx(i);
                if (!cell || !cell->IsShown() || !cell->m_pData)
                    continue;
                Frect cr;
                cell->GetAbsoluteRect(cr);
                if (cr.in(const_cast<Fvector2&>(pos)))
                    return cell;
            }
            continue;
        }

        if (CUICellItem* cell = FindHovered(child, pos))
            return cell;
    }
    return nullptr;
}

void CUIItemHint::Update()
{
    if (!m_info)
        return;

    if (!m_owner->IsShown() || CUIDragDropListEx::m_drag_item)
    {
        Reset();
        return;
    }

    Fvector2 pos = GetUICursor()->GetCursorPosition();
    CUICellItem* cell = FindHovered(m_owner, pos);

    if (cell != m_hovered)
    {
        m_hovered = cell;
        m_hover_start = Device.dwTimeGlobal;
        m_suppressed = nullptr;
        HidePopup();
    }
    if (!cell)
        return;

    // a click hides the popup until the cursor leaves this item
    if (pInput->iGetAsyncKeyState(MOUSE_1) || pInput->iGetAsyncKeyState(MOUSE_1 + 1))
    {
        m_suppressed = cell;
        HidePopup();
        return;
    }
    if (m_suppressed == cell || Device.dwTimeGlobal < m_hover_start + Settings().delay)
        return;

    if (m_shown != cell)
    {
        CInventoryItem* item = static_cast<CInventoryItem*>(cell->m_pData);
        m_info->InitItem(item, CompareItem(item));
        if (m_customize)
            m_customize(*m_info, cell);
        m_info->AdjustHeightToContent();
        m_info->Show(true);
        m_shown = cell;
    }

    // next to the cursor, kept on screen
    Frect r;
    r.set(0.0f, 0.0f, m_info->GetWidth(), m_info->GetHeight());
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
    m_info->SetWndPos(r.x1 - owner_rect.x1, r.y1 - owner_rect.y1);
}
