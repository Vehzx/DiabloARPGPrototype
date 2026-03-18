#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_DamageNumber.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API UWBP_DamageNumber : public UUserWidget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable)
    void SetDamageValue(int32 Damage);

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* DamageText;

    UFUNCTION(BlueprintCallable)
    UWidgetAnimation* GetFloatUpAnimation() const { return FloatUpAnimation; }

protected:

    UPROPERTY(meta = (BindWidgetAnim), Transient)
    UWidgetAnimation* FloatUpAnimation;
};