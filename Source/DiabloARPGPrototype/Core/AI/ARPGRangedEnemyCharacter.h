#pragma once

#include "CoreMinimal.h"
#include "ARPGEnemyCharacter.h"
#include "ARPGRangedEnemyCharacter.generated.h"

class AARPGEnemyProjectile;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGRangedEnemyCharacter : public AARPGEnemyCharacter
{
    GENERATED_BODY()

public:
    AARPGRangedEnemyCharacter();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    virtual bool OverridesChaseMovement() const override { return true; }

    virtual bool CanEnterFlee() const override { return false; }

    bool bFiringDelayActive = false;
    FTimerHandle FiringDelayTimer;
    bool bIsInFiringPosition = false;
    bool bIsRetreating = false;
    float TimeSinceLastShot = 0.f;

    // Firing range
    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    float PreferredMinRange = 450.f;

    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    float PreferredMaxRange = 750.f;

    // Projectile
    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    TSubclassOf<AARPGEnemyProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    float ProjectileDamage = 20.f;

    // Firing timing
    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    float FireCooldown = 2.f;

    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    float FiringDelay = 0.3f;

    // Retreat
    UPROPERTY(EditAnywhere, Category = "Ranged Combat")
    float RetreatStepSize = 300.f;

    void FireProjectile();
    void HandleRangedCombatTick(float DeltaTime);
};