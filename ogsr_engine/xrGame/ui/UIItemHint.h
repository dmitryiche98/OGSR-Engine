#pragma once

// Zone: item description popup near the cursor (like Call of Pripyat).
// Hover a cell item in any drag-drop list of the owner window (inventory, trade, carbody) for `delay` ms ->
// a CUIItemInfo built from its own xml appears next to the cursor, sized to its content, kept on screen.
// Moving off the item, starting a drag or clicking hides it (a click keeps it hidden until the cursor
// leaves that item). Weapons, knives, outfits and helmets are compared with what the actor wears.
// The popup description list is named "item_hint_descr" (CUIWindow::WindowName) for scripts.
//
// system.ltx (external.ltx) section [ui_item_hint]:
//   mode  = panel | popup | both   ; panel - old static description panel only (default), popup - popup only
//                                  ; (the static panel is hidden), both - both
//   xml   = inventory_item_hint.xml
//   delay = 300                    ; ms

#include "UIItemInfo.h"
#include <functional>

class CUIWindow;
class CUICellItem;
class CInventoryItem;

class CUIItemHint
{
public:
    enum EMode
    {
        eModePanel,
        eModePopup,
        eModeBoth
    };
    static EMode Mode();
    static bool PopupEnabled() { return Mode() != eModePanel; }
    static bool PanelEnabled() { return Mode() != eModePopup; }

    // what to compare `item` with: the item of the same kind the actor wears (nullptr - nothing to compare)
    static CInventoryItem* CompareItem(CInventoryItem* item);

    using CUSTOMIZE = std::function<void(CUIItemInfo&, CUICellItem*)>;

    // attaches the popup to `owner` (call at the end of the owner's Init so it is drawn on top)
    void Init(CUIWindow* owner, CUSTOMIZE customize = {});
    void Update(); // call from the owner's Update
    void Reset(); // call from the owner's Show/Hide

private:
    CUIWindow* m_owner{};
    CUIItemInfo* m_info{};
    CUSTOMIZE m_customize;
    CUICellItem* m_hovered{};
    CUICellItem* m_shown{};
    CUICellItem* m_suppressed{};
    u32 m_hover_start{};

    CUICellItem* FindHovered(CUIWindow* w, const Fvector2& pos);
    void HidePopup();
};
