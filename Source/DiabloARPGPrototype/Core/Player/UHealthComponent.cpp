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

void UHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
    if (bIsDead || DamageAmount <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HEALTH] %s cannot take damage (Dead or invalid amount)."),
            *GetOwner()->GetName());
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
        return;

    float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    UE_LOG(LogTemp, Warning, TEXT("[HEALTH] %s took %f damage. Health: %.1f -> %.1f"),
        *Owner->GetName(), DamageAmount, OldHealth, CurrentHealth);

    // Hit flash
    if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(Owner))
    {
        Player->FlashOnHit();
    }
    else if (AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Owner))
    {
        Enemy->FlashOnHit();
    }

    // Knockback
    if (DamageCauser)
    {
        FVector KnockDir = (Owner->GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();

        if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(Owner))
        {
            Player->ApplyKnockback(KnockDir, 400.f);
        }
        else if (AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Owner))
        {
            Enemy->ApplyKnockback(KnockDir, 300.f);
        }
    }

    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HEALTH] %s has died."), *Owner->GetName());
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