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

    // Override stagger to make boss immune
    virtual void EnterStagger(float Duration) override;

    virtual void OnDamaged(AActor* DamageCauser) override;

    virtual bool CanBeKnockedBack() const override { return false; }

    virtual float GetPlayerLeashDistance() const override { return 99999.f; }

protected:
    virtual void BeginPlay() override;
};