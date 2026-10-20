#pragma once

#include "uiwindow.h"
//#include "uipointergage.h"

class CUICarPanel : public CUIWindow
{
private:
    typedef CUIWindow inherited;

    CUIStatic UIStaticCarHealth;
    CUIProgressBar UICarHealthBar;
    // "Загублений в Зоні": шкала бензину (ui\car_fuel.xml, необов'язково)
    CUIStatic UIStaticCarFuel;
    CUIProgressBar UICarFuelBar;
    bool m_bFuel{};
    //	CUIPointerGage		UISpeedometer;
    //	CUIPointerGage		UITachometer;
public:
    // Установить
    void SetCarHealth(float value);
    void SetCarFuel(float value); // 0..1
    void SetSpeed(float speed);
    void SetRPM(float rmp);
    void Init(float x, float y, float width, float height);
};