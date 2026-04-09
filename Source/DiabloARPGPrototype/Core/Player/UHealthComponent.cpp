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
        return;

    AActor* Owner = GetOwner();
    if (!Owner)
        return;

    float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    // Visual hit feedback
    if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(Owner))
    {
        Player->FlashOnHit();
    }
    else if (AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Owner))
    {
        Enemy->FlashOnHit();
        Enemy->EnterStagger(0.3f);
        Enemy->OnDamaged(DamageCauser);
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
            if (Enemy->CanBeKnockedBack())
            {
                Enemy->ApplyKnockback(KnockDir, 300.f);
            }
        }
    }

    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    // Death check
    if (CurrentHealth <= 0.f)
    {
        HandleDeath();
    }
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth)
{
    MaxHealth = NewMaxHealth;
    CurrentHealth = NewMaxHealth;
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::Heal(float HealAmount)
{
    if (bIsDead || HealAmount <= 0.f)
        return;

    CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::ResetHealthToFull()
{
    CurrentHealth = MaxHealth;
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::HandleDeath()
{
    if (bIsDead)
        return;

    bIsDead = true;

    OnDeath.Broadcast();
}