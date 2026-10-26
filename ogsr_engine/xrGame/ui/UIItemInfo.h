#pragma once
#include "uiwindow.h"

class CInventoryItem;
class CUIStatic;
class CUIScrollView;
class CUIProgressBar;
class CUIWpnParams;
class CUIArtefactParams;
class CPhysicsShellHolder;
class CUIFrameWindow;

// extern const char * const 		fieldsCaptionColor;

class CUIItemInfo : public CUIWindow
{
private:
    typedef CUIWindow inherited;
    struct _desc_info
    {
        CGameFont* pDescFont;
        u32 uDescClr;
        bool bShowDescrText;
    } m_desc_info{};
    CInventoryItem* m_pInvItem;

public:
    CUIItemInfo();
    virtual ~CUIItemInfo();

    void Init(float x, float y, float width, float height, LPCSTR xml_name);
    void Init(LPCSTR xml_name);
    // pCompareItem (Zone): what the actor wears of the same kind - weapon params show it as second bars,
    // the script callback gets it as the 3rd argument
    void InitItem(CInventoryItem* pInvItem, CInventoryItem* pCompareItem = nullptr);
    // Zone: shrink the description list (and the window, background_frame) to the content, for the popup
    void AdjustHeightToContent();

    void TryAddWpnInfo(CPhysicsShellHolder& obj, CPhysicsShellHolder* cmp = nullptr);
    void TryAddArtefactInfo(const shared_str& af_section);
    void TryAddCustomInfo(CPhysicsShellHolder& obj, CPhysicsShellHolder* cmp = nullptr);

    virtual void Draw();
    bool m_b_force_drawing;
    CUIStatic* UIName;
    CUIStatic* UIWeight;
    CUIStatic* UICost;
    CUIStatic* UICondition;
    CUIScrollView* UIDesc;
    CUIProgressBar* UICondProgresBar;
    CUIWpnParams* UIWpnParams;
    CUIArtefactParams* UIArtefactParams;

    Fvector2 UIItemImageSize;
    CUIStatic* UIItemImage;

    CUIFrameWindow* UIBackground{}; // Zone: optional node background_frame, resized by AdjustHeightToContent
    float m_descMaxHeight{};
    float m_bottomIndent{};
};