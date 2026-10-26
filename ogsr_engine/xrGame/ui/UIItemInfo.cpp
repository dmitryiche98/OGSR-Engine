#include "stdafx.h"

#include "uiiteminfo.h"
#include "uistatic.h"
#include "UIXmlInit.h"

#include "UIListWnd.h"
#include "UIProgressBar.h"
#include "UIScrollView.h"

#include "../string_table.h"
#include "../Inventory_Item.h"
#include "UIInventoryUtilities.h"
#include "../PhysicsShellHolder.h"
#include "UIWpnParams.h"
#include "ui_af_params.h"
#include "UIFrameWindow.h"
#include <format>

CUIItemInfo::CUIItemInfo()
{
    UIItemImageSize.set(0.0f, 0.0f);
    UICondProgresBar = NULL;
    UICondition = NULL;
    UICost = NULL;
    UIWeight = NULL;
    UIItemImage = NULL;
    UIDesc = NULL;
    UIWpnParams = NULL;
    UIArtefactParams = NULL;
    UIName = NULL;
    m_pInvItem = NULL;
    m_b_force_drawing = false;
}

CUIItemInfo::~CUIItemInfo()
{
    xr_delete(UIWpnParams);
    xr_delete(UIArtefactParams);
}

void CUIItemInfo::Init(LPCSTR xml_name)
{
    CUIXml uiXml;
    bool xml_result = uiXml.Init(CONFIG_PATH, UI_PATH, xml_name);
    R_ASSERT2(xml_result, "xml file not found");

    CUIXmlInit xml_init;

    if (uiXml.NavigateToNode("main_frame", 0))
    {
        Frect wnd_rect;
        wnd_rect.x1 = uiXml.ReadAttribFlt("main_frame", 0, "x", 0);
        wnd_rect.y1 = uiXml.ReadAttribFlt("main_frame", 0, "y", 0);

        wnd_rect.x2 = uiXml.ReadAttribFlt("main_frame", 0, "width", 0);
        wnd_rect.y2 = uiXml.ReadAttribFlt("main_frame", 0, "height", 0);

        inherited::Init(wnd_rect.x1, wnd_rect.y1, wnd_rect.x2, wnd_rect.y2);
        m_bottomIndent = uiXml.ReadAttribFlt("main_frame", 0, "bottom_indent", 0);
    }

    // Zone: frame behind everything else (attached first), stretched to the content height in the popup
    if (uiXml.NavigateToNode("background_frame", 0))
    {
        UIBackground = xr_new<CUIFrameWindow>();
        AttachChild(UIBackground);
        UIBackground->SetAutoDelete(true);
        CUIXmlInit::InitFrameWindow(uiXml, "background_frame", 0, UIBackground);
    }

    if (uiXml.NavigateToNode("static_name", 0))
    {
        UIName = xr_new<CUIStatic>();
        AttachChild(UIName);
        UIName->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_name", 0, UIName);
    }
    if (uiXml.NavigateToNode("static_weight", 0))
    {
        UIWeight = xr_new<CUIStatic>();
        AttachChild(UIWeight);
        UIWeight->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_weight", 0, UIWeight);
    }

    if (uiXml.NavigateToNode("static_cost", 0))
    {
        UICost = xr_new<CUIStatic>();
        AttachChild(UICost);
        UICost->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_cost", 0, UICost);
    }

    if (uiXml.NavigateToNode("static_condition", 0))
    {
        UICondition = xr_new<CUIStatic>();
        AttachChild(UICondition);
        UICondition->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "static_condition", 0, UICondition);
    }

    if (uiXml.NavigateToNode("condition_progress", 0))
    {
        UICondProgresBar = xr_new<CUIProgressBar>();
        AttachChild(UICondProgresBar);
        UICondProgresBar->SetAutoDelete(true);
        xml_init.InitProgressBar(uiXml, "condition_progress", 0, UICondProgresBar);
    }

    if (uiXml.NavigateToNode("descr_list", 0))
    {
        UIWpnParams = xr_new<CUIWpnParams>();
        UIWpnParams->InitFromXml(uiXml);

        UIArtefactParams = xr_new<CUIArtefactParams>();
        UIArtefactParams->InitFromXml(uiXml);

        UIDesc = xr_new<CUIScrollView>();
        AttachChild(UIDesc);
        UIDesc->SetAutoDelete(true);
        xml_init.InitScrollView(uiXml, "descr_list", 0, UIDesc);
        m_descMaxHeight = UIDesc->GetHeight();

        m_desc_info.bShowDescrText = !!uiXml.ReadAttribInt("descr_list", 0, "only_text_info", 1);
        xml_init.InitFont(uiXml, "descr_list:font", 0, m_desc_info.uDescClr, m_desc_info.pDescFont);
    }

    if (uiXml.NavigateToNode("image_static", 0))
    {
        UIItemImage = xr_new<CUIStatic>();
        AttachChild(UIItemImage);
        UIItemImage->SetAutoDelete(true);
        xml_init.InitStatic(uiXml, "image_static", 0, UIItemImage);
        UIItemImage->TextureAvailable(true);

        UIItemImage->TextureOff();
        UIItemImage->ClipperOn();
        UIItemImageSize.set(UIItemImage->GetWidth(), UIItemImage->GetHeight());
    }

    xml_init.InitAutoStaticGroup(uiXml, "auto", 0, this);
}

void CUIItemInfo::Init(float x, float y, float width, float height, LPCSTR xml_name)
{
    inherited::Init(x, y, width, height);
    Init(xml_name);
}

void CUIItemInfo::InitItem(CInventoryItem* pInvItem, CInventoryItem* pCompareItem)
{
    m_pInvItem = pInvItem;
    if (!m_pInvItem)
        return;

    if (UIName)
    {
        UIName->SetText(pInvItem->Name());
    }
    if (UIWeight)
    {
        string256 str;
        sprintf_s(str, "%3.2f kg", pInvItem->Weight());
        UIWeight->SetText(str);
    }
    if (UICost)
    {
        static const char* StMoneyDescr = CStringTable().translate("ui_st_money_descr").c_str();  
        UICost->SetText(std::format("{} {}", pInvItem->Cost(), StMoneyDescr).c_str());
    }

    if (UICondProgresBar)
    {
        float cond = pInvItem->GetConditionToShow();
        UICondProgresBar->Show(true);
        UICondProgresBar->SetProgressPos(cond * 100.0f + 1.0f - EPS);
    }

    if (UIDesc)
    {
        UIDesc->Clear();
        VERIFY(0 == UIDesc->GetSize());
        CPhysicsShellHolder* cmp = pCompareItem ? &pCompareItem->object() : nullptr;
        TryAddWpnInfo(pInvItem->object(), cmp);
        TryAddArtefactInfo(pInvItem->object().cNameSect());
        TryAddCustomInfo(pInvItem->object(), cmp);
        if (m_desc_info.bShowDescrText)
        {
            CUIStatic* pItem = xr_new<CUIStatic>();
            pItem->SetTextColor(m_desc_info.uDescClr);
            pItem->SetFont(m_desc_info.pDescFont);
            pItem->SetWidth(UIDesc->GetDesiredChildWidth());
            pItem->SetTextComplexMode(true);
            pItem->SetText(*pInvItem->ItemDescription());
            pItem->AdjustHeightToText();
            UIDesc->AddWindow(pItem, true);
        }
        UIDesc->ScrollToBegin();
    }
    if (UIItemImage)
    {
        // Загружаем картинку
        pInvItem->m_icon_params.set_shader(UIItemImage);

        Frect rect = pInvItem->m_icon_params.original_rect();
        UIItemImage->TextureOn();
        UIItemImage->ClipperOn();
        Frect v_r = {0.0f, 0.0f, rect.width(), rect.height()};
        v_r.x2 *= UI()->get_current_kx();

        UIItemImage->GetUIStaticItem().SetRect(v_r);
        UIItemImage->SetWidth(_min(v_r.width(), UIItemImageSize.x));
        UIItemImage->SetHeight(_min(v_r.height(), UIItemImageSize.y));
    }
}

void CUIItemInfo::TryAddWpnInfo(CPhysicsShellHolder& obj, CPhysicsShellHolder* cmp)
{
    if (UIWpnParams->Check(obj))
    {
        UIWpnParams->SetInfo(obj, cmp && UIWpnParams->Check(*cmp) ? cmp : nullptr);
        UIDesc->AddWindow(UIWpnParams, false);
    }
}

void CUIItemInfo::TryAddArtefactInfo(const shared_str& af_section)
{
    if (UIArtefactParams->Check(af_section))
    {
        UIArtefactParams->SetInfo(af_section);
        UIDesc->AddWindow(UIArtefactParams, false);
    }
}

#include "script_game_object.h"

void CUIItemInfo::TryAddCustomInfo(CPhysicsShellHolder& obj, CPhysicsShellHolder* cmp)
{
    if (pSettings->line_exist("engine_callbacks", "ui_item_info_callback"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "ui_item_info_callback");
        if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
        {
            // Zone: 3rd argument - the item to compare with (nil if none)
            lua_function(UIDesc, obj.lua_game_object(), cmp ? cmp->lua_game_object() : nullptr);
        }
    }
}

void CUIItemInfo::AdjustHeightToContent()
{
    if (!UIDesc)
        return;
    const float h = _min(UIDesc->GetContentHeight(), m_descMaxHeight);
    UIDesc->SetHeight(h);
    UIDesc->ScrollToBegin();
    const float total = UIDesc->GetWndPos().y + h + m_bottomIndent;
    SetHeight(total);
    if (UIBackground)
        UIBackground->SetHeight(total);
}

void CUIItemInfo::Draw()
{
    if (m_pInvItem || m_b_force_drawing)
        inherited::Draw();
}
