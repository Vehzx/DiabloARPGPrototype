#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
}

void UHealthComponent::ApplyDamage(float DamageAmount)
{
    if (bIsDead || DamageAmount <= 0.f)
        return;

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
    {
        HandleDeath();
    }
}

void UHealthComponent::Heal(float HealAmount)
{
    if (bIsDead || HealAmount <= 0.f)
        return;

    CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::HandleDeath()
{
    if (bIsDead)
        return;

    bIsDead = true;

    OnDeath.Broadcast();

    // Optional: destroy actor after delay
    // GetOwner()->Destroy();
}