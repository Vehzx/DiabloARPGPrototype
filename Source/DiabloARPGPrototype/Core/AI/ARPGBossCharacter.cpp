#include "ARPGBossCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionComponent.h"

AARPGBossCharacter::AARPGBossCharacter()
{
    // Larger scale
    SetActorScale3D(FVector(3.f, 3.f, 3.f));

    // Slower movement
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 150.f;
    }

    // Higher health — override in BeginPlay via HealthComponent
    // Larger attack range to match bigger size
    AttackRange = 250.f;

    // Slower attack cooldown — hits harder, hits slower
    AttackCooldown = 2.0f;

    // Larger attack damage
    AttackDamage = 40.f;

    // Larger leash radius for boss room
    LeashRadius = 3000.f;
}

void AARPGBossCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HealthComponent)
        HealthComponent->SetMaxHealth(500.f);

    // Override movement speed
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        MoveComp->MaxWalkSpeed = 250.f;

    // Override sight perception
    if (SightConfig)
    {
        SightConfig->SightRadius = 2000.f;
        SightConfig->LoseSightRadius = 2100.f;
        SightConfig->PeripheralVisionAngleDegrees = 180.f;

        if (PerceptionComponent)
            PerceptionComponent->ConfigureSense(*SightConfig);
    }

    // Boss never leashes, once aggro it always chases
    LeashRadius = 99999.f;
}

void AARPGBossCharacter::OnDamaged(AActor* DamageCauser)
{
    if (!DamageCauser || CurrentState == EEnemyState::Dead)
        return;

    CurrentTarget = DamageCauser;

    // Boss never panics or flees — always chases
    if (CurrentState != EEnemyState::Chase &&
        CurrentState != EEnemyState::Attack)
    {
        SetEnemyState(EEnemyState::Chase);
    }
}

void AARPGBossCharacter::EnterStagger(float Duration)
{
    // Boss is immune to stagger — do nothing
    UE_LOG(LogTemp, Warning, TEXT("[BOSS] Stagger ignored — boss is immune"));
}