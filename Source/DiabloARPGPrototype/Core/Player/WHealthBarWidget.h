#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WHealthBarWidget.generated.h"

class UProgressBar;
class UHealthComponent;

UCLASS()
class DIABLOARPGPROTOTYPE_API UWHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeHealth(UHealthComponent* InHealthComponent);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthProgressBar;

    UFUNCTION()
    void OnHealthChanged(float NewHealth, float MaxHealth);

    UHealthComponent* HealthComponent;
};