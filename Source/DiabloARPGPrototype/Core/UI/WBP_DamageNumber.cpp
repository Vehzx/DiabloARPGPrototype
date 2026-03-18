#include "WBP_DamageNumber.h"
#include "Components/TextBlock.h"

void UWBP_DamageNumber::SetDamageValue(int32 Damage)
{
    if (DamageText)
    {
        DamageText->SetText(FText::AsNumber(Damage));
    }
}