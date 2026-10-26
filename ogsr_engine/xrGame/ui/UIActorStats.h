#pragma once

// Zone: player stats block in the inventory (health, psy, stamina; satiety, water, bodrost; bleeding, radiation, alcohol).
// The engine only owns the list and the hover hint; rows, texts, colours and wording are made by a Lua script:
//   system.ltx [engine_callbacks] ui_actor_stats_callback = actor_stats.fill   ; fill(list) - CUIScrollView
// The script adds CUIStatic rows to the list (AddWindow(st, false, false)), the list is cleared before each call.
// A row whose WindowName is set (string table id) shows that text as a hint next to the cursor on hover.
//
// inventory xml (optional - without the node nothing is created):
//   <actor_stats x= y= width= height= period="250" delay="300">  ... scroll view attributes ...
//   <actor_stats_hint width="260" height="20" pad="6">   ... static: <text font= r= g= b=/> (width = text width)
//       <frame><texture>ui_icons_PDA_dialog</texture></frame>   ... optional background frame
//   </actor_stats_hint>
// period - ms between script refreshes, delay - ms of hovering before the hint appears.

class CUIWindow;
class CUIScrollView;
class CUIStatic;
class CUIXml;
class CUIFrameWindow;

class CUIActorStats
{
public:
    // false - the xml has no actor_stats node
    bool Init(CUIXml& xml, CUIWindow* owner);
    void Update();
    void Reset();

private:
    CUIWindow* m_owner{};
    CUIScrollView* m_list{};
    CUIStatic* m_hint{};
    CUIFrameWindow* m_hint_frame{};
    float m_hint_pad{};
    u32 m_period{250};
    u32 m_delay{300};
    u32 m_next_fill{};
    CUIWindow* m_hovered{};
    shared_str m_hovered_name;
    u32 m_hover_start{};
    bool m_hint_shown{};

    void Fill();
    CUIWindow* FindHovered(const Fvector2& pos);
    void ShowHint(const Fvector2& pos);
    void HideHint();
};
