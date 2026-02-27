#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "DiabloARPGPrototype/Core/AI/ARPGEnemyCharacter.h"
#include "DiabloARPGPrototype/Core/Player/ARPGPlayerCharacter.h"
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
    {
        UE_LOG(LogTemp, Warning, TEXT("[HEALTH] %s cannot take damage (Dead or invalid amount)."),
            *GetOwner()->GetName());
        return;
    }

    float OldHealth = CurrentHealth;

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    UE_LOG(LogTemp, Warning, TEXT("[HEALTH] %s took %f damage. Health: %.1f -> %.1f"),
        *GetOwner()->GetName(),
        DamageAmount,
        OldHealth,
        CurrentHealth
    );

    // --- HIT FLASH HERE ---
    if (AActor* Owner = GetOwner())
    {
        // Player
        if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(Owner))
        {
            Player->FlashOnHit();
        }

        // Enemy
        else if (AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Owner))
        {
            Enemy->FlashOnHit();
        }
    }
    // -----------------------

    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HEALTH] %s has died."), *GetOwner()->GetName());
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