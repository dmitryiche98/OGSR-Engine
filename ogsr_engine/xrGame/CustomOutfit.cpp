#include "stdafx.h"

#include "customoutfit.h"
#include "PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "..\Include/xrRender/Kinematics.h"
#include "../Include/xrRender/RenderVisual.h"
#include "UIGameSP.h"
#include "HudManager.h"
#include "ui/UIInventoryWnd.h"
#include "player_hud.h"
#include "xrserver_objects_alife_items.h"

CCustomOutfit::CCustomOutfit()
{
    SetSlot(OUTFIT_SLOT);

    m_flags.set(FUsingCondition, TRUE);

    m_HitTypeProtection.resize(ALife::eHitTypeMax);
    for (int i = 0; i < ALife::eHitTypeMax; i++)
        m_HitTypeProtection[i] = 1.0f;

    m_boneProtection = xr_new<SBoneProtections>();

    m_fBleedingRestoreSpeed = 0.f;
    m_fHealthRestoreSpeed = 0.f;
    m_fPowerRestoreSpeed = 0.f;
    m_fSatietyRestoreSpeed = 0.f;
    m_fThirstRestoreSpeed = 0.f;
}

CCustomOutfit::~CCustomOutfit() { xr_delete(m_boneProtection); }

void CCustomOutfit::net_Export(CSE_Abstract* E)
{
    inherited::net_Export(E);
    CSE_ALifeInventoryItem* itm = smart_cast<CSE_ALifeInventoryItem*>(E);
    itm->m_fCondition = m_fCondition;
}

void CCustomOutfit::Load(LPCSTR section)
{
    inherited::Load(section);

    m_HitTypeProtection[ALife::eHitTypeBurn] = pSettings->r_float(section, "burn_protection");
    m_HitTypeProtection[ALife::eHitTypeStrike] = pSettings->r_float(section, "strike_protection");
    m_HitTypeProtection[ALife::eHitTypeShock] = pSettings->r_float(section, "shock_protection");
    m_HitTypeProtection[ALife::eHitTypeWound] = pSettings->r_float(section, "wound_protection");
    m_HitTypeProtection[ALife::eHitTypeRadiation] = pSettings->r_float(section, "radiation_protection");
    m_HitTypeProtection[ALife::eHitTypeTelepatic] = pSettings->r_float(section, "telepatic_protection");
    m_HitTypeProtection[ALife::eHitTypeChemicalBurn] = pSettings->r_float(section, "chemical_burn_protection");
    m_HitTypeProtection[ALife::eHitTypeExplosion] = pSettings->r_float(section, "explosion_protection");
    m_HitTypeProtection[ALife::eHitTypeFireWound] = pSettings->r_float(section, "fire_wound_protection");
    m_HitTypeProtection[ALife::eHitTypePhysicStrike] = READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection", 0.0f);

    if (pSettings->line_exist(section, "actor_visual"))
        m_ActorVisual = pSettings->r_string(section, "actor_visual");
    else
        m_ActorVisual = NULL;

    m_ef_equipment_type = pSettings->r_u32(section, "ef_equipment_type");
    if (pSettings->line_exist(section, "power_loss"))
        m_fPowerLoss = pSettings->r_float(section, "power_loss");
    else
        m_fPowerLoss = 1.0f;

    m_bBuiltInHelmet = READ_IF_EXISTS(pSettings, r_bool, section, "built_in_helmet", false);
    m_fHeadFireWoundArmour = READ_IF_EXISTS(pSettings, r_float, section, "head_fire_wound_armour", -1.f);
    m_fHeadStrikeProtection = READ_IF_EXISTS(pSettings, r_float, section, "head_strike_protection", -1.f);
    m_fHeadWoundProtection = READ_IF_EXISTS(pSettings, r_float, section, "head_wound_protection", -1.f);

    m_additional_weight = pSettings->r_float(section, "additional_inventory_weight");
    m_additional_weight2 = pSettings->r_float(section, "additional_inventory_weight2");

    if (pSettings->line_exist(section, "nightvision_sect"))
        m_NightVisionSect = pSettings->r_string(section, "nightvision_sect");
    else
        m_NightVisionSect = NULL;

    m_full_icon_name = pSettings->r_string(section, "full_icon_name");

    m_fBleedingRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed", 0.f);
    m_fHealthRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed", 0.f);
    m_fPowerRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed", 0.f);
    m_fSatietyRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed", 0.f);
    m_fThirstRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "thirst_restore_speed", 0.f);

    m_artefact_count = READ_IF_EXISTS(pSettings, r_u32, section, "artefact_count", pSettings->r_u32("inventory", "max_belt"));
}

void CCustomOutfit::Hit(float hit_power, ALife::EHitType hit_type)
{
    hit_power *= m_HitTypeK[hit_type];

    if (hit_power > 0)
    {
        ChangeCondition(-hit_power);
    }
}

float CCustomOutfit::GetDefHitTypeProtection(ALife::EHitType hit_type) { return 1.0f - m_HitTypeProtection[hit_type] * GetCondition(); }

float CCustomOutfit::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
    float fBase = m_HitTypeProtection[hit_type] * GetCondition();
    float bone = m_boneProtection->getBoneProtection(element);
    return 1.0f - fBase * bone;
}

// "Загублений в Зоні": бронебійність обмежуємо 0..1, щоб броня не додавала шкоди при k_ap > 1
static float clamp_ap(float AP) { return AP < 0.f ? 0.f : (AP > 1.f ? 1.f : AP); }

float CCustomOutfit::GetBoneArmourRaw(s16 bone) { return m_boneProtection->getBoneArmour(bone); }

float CCustomOutfit::GetHeadArmourRaw(s16 head_bone) { return m_fHeadFireWoundArmour >= 0.f ? m_fHeadFireWoundArmour : m_boneProtection->getBoneArmour(head_bone); }

float CCustomOutfit::HeadHitThruArmour(float hit_power, s16 element, float AP)
{
    const float armour = GetHeadArmourRaw(element) * GetCondition() * (1.f - clamp_ap(AP));
    const float min_hit = hit_power * m_boneProtection->m_fHitFrac;
    const float r = hit_power - armour;
    return r < min_hit ? min_hit : r;
}

float CCustomOutfit::GetHeadHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
    float prot = -1.f;
    if (hit_type == ALife::eHitTypeStrike)
        prot = m_fHeadStrikeProtection;
    else if (hit_type == ALife::eHitTypeWound || hit_type == ALife::eHitTypeWound_2)
        prot = m_fHeadWoundProtection;
    if (prot < 0.f)
        return GetHitTypeProtection(hit_type, element);
    return 1.f - prot * GetCondition();
}

float CCustomOutfit::HitThruArmour(float hit_power, s16 element, float AP)
{
    float BoneArmour = m_boneProtection->getBoneArmour(element) * GetCondition() * (1 - clamp_ap(AP));
    float NewHitPower = hit_power - BoneArmour;
    if (NewHitPower < hit_power * m_boneProtection->m_fHitFrac)
        return hit_power * m_boneProtection->m_fHitFrac;
    return NewHitPower;
};

#include "torch.h"

void CCustomOutfit::OnMoveToSlot()
{
    inherited::OnMoveToSlot();
    if (m_pCurrentInventory)
    {
        CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
        if (pActor)
        {
            if (m_ActorVisual.size())
            {
                pActor->ChangeVisual(m_ActorVisual);
            }
            if (pSettings->line_exist(cNameSect(), "bones_koeff_protection"))
            {
                m_boneProtection->reload(pSettings->r_string(cNameSect(), "bones_koeff_protection"), smart_cast<IKinematics*>(pActor->Visual()));
            }

            if (pSettings->line_exist(cNameSect(), "player_hud_section"))
                g_player_hud->load(pSettings->r_string(cNameSect(), "player_hud_section"));
            else
                g_player_hud->load_default();

            smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame())->InventoryMenu->UpdateOutfit();

            // костюм із вбудованим шоломом: окремий шолом знімаємо в рюкзак
            if (m_bBuiltInHelmet)
            {
                PIItem helm = m_pCurrentInventory->m_slots[HELMET_SLOT].m_pIItem;
                if (helm && m_pCurrentInventory->Ruck(helm))
                    smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame())->InventoryMenu->InitInventory_delayed();
            }
        }
    }
}

void CCustomOutfit::OnDropOrMoveToRuck()
{
    if (m_pCurrentInventory && !Level().is_removing_objects())
    {
        CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
        if (pActor)
        {
            // ПНВ тепер не в ліхтарі: CActor::UpdateNightVision сам вимкне його, коли зникне джерело
            if (m_ActorVisual.size())
            {
                shared_str DefVisual = pActor->GetDefaultVisualOutfit();
                if (DefVisual.size())
                {
                    pActor->ChangeVisual(DefVisual);
                }
            }

            g_player_hud->load_default();

            smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame())->InventoryMenu->UpdateOutfit();
        }
    }
}

void CCustomOutfit::OnMoveToRuck(EItemPlace prevPlace)
{
    inherited::OnMoveToRuck(prevPlace);

    OnDropOrMoveToRuck();
}

void CCustomOutfit::OnDrop()
{
    inherited::OnDrop();

    OnDropOrMoveToRuck();
}

u32 CCustomOutfit::ef_equipment_type() const { return (m_ef_equipment_type); }

float CCustomOutfit::GetPowerLoss()
{
    if (m_fPowerLoss < 1 && GetCondition() <= 0)
    {
        return 1.0f;
    };
    return m_fPowerLoss;
};
