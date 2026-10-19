#include "StdAfx.h"
#include "UIOutfitInfo.h"
#include "UIXmlInit.h"
#include "UIStatic.h"
#include "UIScrollView.h"
#include "../actor.h"
#include "../CustomOutfit.h"
#include "../string_table.h"

CUIOutfitInfo::CUIOutfitInfo() {}

CUIOutfitInfo::~CUIOutfitInfo()
{
    for (u32 i = _item_start; i < _max_item_index; ++i)
    {
        CUIStatic* _s = m_items[i];
        xr_delete(_s);
    }
}

LPCSTR _imm_names[] = {
    "health_restore_speed", "radiation_restore_speed", "satiety_restore_speed", "thirst_restore_speed", "power_restore_speed", "bleeding_restore_speed", "psy_health_restore_speed",

    "burn_immunity",        "shock_immunity",          "strike_immunity",       "wound_immunity",       "radiation_immunity",  "telepatic_immunity",     "chemical_burn_immunity",
    "explosion_immunity",   "fire_wound_immunity",
};

LPCSTR _imm_st_names[] = {
    "ui_inv_health",
    "ui_inv_radiation",
    "ui_inv_satiety",
    "ui_inv_thirst",
    "ui_inv_power",
    "ui_inv_bleeding",
    "ui_inv_psy_health",

    "ui_inv_outfit_burn_protection",
    "ui_inv_outfit_shock_protection",
    "ui_inv_outfit_strike_protection",
    "ui_inv_outfit_wound_protection",
    "ui_inv_outfit_radiation_protection",
    "ui_inv_outfit_telepatic_protection",
    "ui_inv_outfit_chemical_burn_protection",
    "ui_inv_outfit_explosion_protection",
    "ui_inv_outfit_fire_wound_protection",
};

LPCSTR _actor_param_names[] = {"satiety_health_v", "radiation_v", "satiety_v", "thirst_v", "satiety_power_v", "wound_incarnation_v", "psy_health_v"};

void CUIOutfitInfo::InitFromXml(CUIXml& xml_doc)
{
    LPCSTR _base = "outfit_info";

    string256 _buff;
    CUIXmlInit::InitWindow(xml_doc, _base, 0, this);

    m_listWnd = xr_new<CUIScrollView>();
    m_listWnd->SetAutoDelete(true);
    AttachChild(m_listWnd);
    strconcat(sizeof(_buff), _buff, _base, ":scroll_view");
    CUIXmlInit::InitScrollView(xml_doc, _buff, 0, m_listWnd);

    for (u32 i = _item_start; i < _max_item_index; ++i)
    {
        strconcat(sizeof(_buff), _buff, _base, ":static_", _imm_names[i]);

        if (xml_doc.NavigateToNode(_buff, 0))
        {
            m_items[i] = xr_new<CUIStatic>();
            CUIStatic* _s = m_items[i];
            _s->SetAutoDelete(false);
            CUIXmlInit::InitStatic(xml_doc, _buff, 0, _s);
        }
    }
}

float CUIOutfitInfo::GetArtefactParam(ActorRestoreParams params, u32 i)
{
    float r = 0;
    switch (i)
    {
    case _item_health_restore_speed: r = params.HealthRestoreSpeed; break;
    case _item_radiation_restore_speed: r = params.RadiationRestoreSpeed; break;
    case _item_satiety_restore_speed: r = params.SatietyRestoreSpeed; break;
    case _item_thirst_restore_speed: r = params.ThirstRestoreSpeed; break;
    case _item_power_restore_speed: r = params.PowerRestoreSpeed; break;
    case _item_bleeding_restore_speed: r = params.BleedingRestoreSpeed; break;
    case _item_psy_health_restore_speed: r = params.PsyHealthRestoreSpeed; break;
    }
    return r;
}

#include "script_game_object.h"

void CUIOutfitInfo::Update(CCustomOutfit* outfit)
{
    string128 _buff;

    auto artefactEffects = Actor()->ActiveArtefactsOnBelt();

    m_listWnd->Clear(false); // clear existing items and do not scroll to top

    // "Загублений в Зоні": порядок рядків - від найчастіших загроз до найрідших, далі відновлення від артефактів.
    // Індекс захисту = _max_item_index1 + ALife::EHitType.
    static const u32 s_order[_max_item_index] = {
        _max_item_index1 + ALife::eHitTypeFireWound, _max_item_index1 + ALife::eHitTypeWound,     _max_item_index1 + ALife::eHitTypeStrike,
        _max_item_index1 + ALife::eHitTypeExplosion, _max_item_index1 + ALife::eHitTypeRadiation, _max_item_index1 + ALife::eHitTypeBurn,
        _max_item_index1 + ALife::eHitTypeChemicalBurn, _max_item_index1 + ALife::eHitTypeShock,  _max_item_index1 + ALife::eHitTypeTelepatic,
        _item_health_restore_speed, _item_bleeding_restore_speed, _item_radiation_restore_speed, _item_power_restore_speed,
        _item_satiety_restore_speed, _item_thirst_restore_speed, _item_psy_health_restore_speed,
    };

    for (u32 oi = 0; oi < _max_item_index; ++oi)
    {
        const u32 i = s_order[oi];
        CUIStatic* _s = m_items[i];

        if (!_s)
            continue;

        float _val_outfit = 0.0f;
        float _val_af = 0.0f;
        float _val_head = 0.0f;
        bool _split = false; // окремі рядки "(тіло)" і "(голова)"

        if (i < _max_item_index1)
        {
            _val_outfit = GetArtefactParam(artefactEffects, i);

            float _actor_val = pSettings->r_float("actor_condition", _actor_param_names[i]);
            _val_outfit = (_val_outfit / _actor_val);
        }
        else
        {
            // "Загублений в Зоні": одна сумарна цифра замість "костюм + артефакти/шолом".
            // Шкода множиться послідовно: на коефіцієнт артефактів і шолома (Actor::Hit), потім костюма
            // (ConditionHit), тож сумарний захист = 1 - k_костюма * k_артефактів. Обидва k >= 0,
            // тому більше 100% не буває; від'ємне значення - предмети підсилюють шкоду.
            // Для куль/ударів/порізів (head_only) шолом діє лише на голову, тож показуємо два рядки:
            //   тіло   = 1 - k_костюма * k_артефактів;
            //   голова = 1 - k_костюма_для_голови * k_артефактів_і_шолома,
            // де костюм для голови рахується, лише якщо має вбудований шолом або outfit_protects_head.
            // Артефакти діють на все тіло - вони в обох рядках.
            const ALife::EHitType ht = ALife::EHitType(i - _max_item_index1);
            float k_outfit = outfit ? outfit->GetDefHitTypeProtection(ht) : 1.0f;
            if (k_outfit < 0.0f)
                k_outfit = 0.0f;
            const float k_af_helm = Actor()->HitArtefactsOnBelt(1.0f, ht); // артефакти на поясі + шолом, вже >= 0

            if (Actor()->HelmetParams().head_only[ht])
            {
                const float k_af = Actor()->HitArtefactsOnBelt(1.0f, ht, true); // лише артефакти
                const float k_outfit_head = Actor()->OutfitProtectsHead() ? k_outfit : 1.0f;
                _val_outfit = 1.0f - k_outfit * k_af;
                _val_head = 1.0f - k_outfit_head * k_af_helm;
                _split = !fsimilar(_val_outfit, _val_head, 0.005f);
            }
            else
                _val_outfit = 1.0f - k_outfit * k_af_helm;
            _val_af = 0.0f;
        }

        if (fsimilar(_val_outfit, 0.0f) && fsimilar(_val_af, 0.0f) && !_split)
        {
            continue;
        }

        LPCSTR _sn = "";
        if (i != _item_radiation_restore_speed && i != _item_power_restore_speed)
        {
            _val_outfit *= 100.0f;
            _val_af *= 100.0f;
            _sn = "%";
        }

        if (i == _item_bleeding_restore_speed)
            _val_outfit *= -1.0f;

        LPCSTR _color = (_val_outfit > 0) ? "%c[green]" : "%c[red]";

        if (i == _item_bleeding_restore_speed || i == _item_radiation_restore_speed)
            _color = (_val_outfit > 0) ? "%c[red]" : "%c[green]";

        LPCSTR _imm_name = *CStringTable().translate(_imm_st_names[i]);

        int _sz = 0;
        if (_split)
        {
            // один рядок: "Назва: голова +50%, тіло +30%"
            _val_head *= 100.0f;
            _sz = sprintf_s(_buff, sizeof(_buff), "%s: %s %s%+.0f%%%s, %s %s%+.0f%%", _imm_name, *CStringTable().translate("ui_inv_prot_head"),
                            (_val_head > 0) ? "%c[green]" : "%c[red]", _val_head, "%c[default]", *CStringTable().translate("ui_inv_prot_body"), _color, _val_outfit);
        }
        else
        {
            _sz = sprintf_s(_buff, sizeof(_buff), "%s ", _imm_name);
            _sz += sprintf_s(_buff + _sz, sizeof(_buff) - _sz, "%s %+3.0f%s", _color, _val_outfit, _sn);
        }

        if (!fsimilar(_val_af, 0.0f))
        {
            _sz += sprintf_s(_buff + _sz, sizeof(_buff) - _sz, "%s %+3.0f%%", (_val_af > 0.0f) ? "%c[green]" : "%c[red]", _val_af);
        }

        _s->SetText(_buff);

        m_listWnd->AddWindow(_s, false);
    }

    if (pSettings->line_exist("engine_callbacks", "ui_actor_info_callback"))
    {
        const char* callback = pSettings->r_string("engine_callbacks", "ui_actor_info_callback");
        if (luabind::functor<void> lua_function; ai().script_engine().functor(callback, lua_function))
        {
            lua_function(m_listWnd, outfit ? outfit->lua_game_object() : nullptr);
        }
    }
}
