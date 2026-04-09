#pragma once

#include "CoreMinimal.h"
#include "ARPGEnemyCharacter.h"
#include "ARPGBossCharacter.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGBossCharacter : public AARPGEnemyCharacter
{
    GENERATED_BODY()

public:
    AARPGBossCharacter();

    virtual void EnterStagger(float Duration) override;

    virtual void OnDamaged(AActor* DamageCauser) override;

    virtual bool CanBeKnockedBack() const override { return false; }

    virtual float GetPlayerLeashDistance() const override { return 99999.f; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> YouWinWidgetClass;

    virtual void HandleDeath() override;

private:
    UPROPERTY()
    UUserWidget* YouWinWidgetInstance;
};