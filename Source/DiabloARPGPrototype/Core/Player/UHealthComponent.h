#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UHealthComponent.generated.h"

// ------------------------------------------------------------
// Delegates
// ------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIABLOARPGPROTOTYPE_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHealthComponent();

    // ------------------------------------------------------------
    // Health API
    // ------------------------------------------------------------
    void ApplyDamage(float DamageAmount);
    void Heal(float HealAmount);

    float GetHealth() const { return CurrentHealth; }
    float GetMaxHealth() const { return MaxHealth; }

    bool IsDead() const { return bIsDead; }

    // ------------------------------------------------------------
    // Events
    // ------------------------------------------------------------
    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable)
    FOnDeath OnDeath;

protected:
    virtual void BeginPlay() override;

private:

    // ------------------------------------------------------------
    // Health Data
    // ------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Health")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "Health")
    float CurrentHealth = 0.f;

    bool bIsDead = false;

    void HandleDeath();
};