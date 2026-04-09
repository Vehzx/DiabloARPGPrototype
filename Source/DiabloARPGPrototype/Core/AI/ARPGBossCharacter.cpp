#include "ARPGBossCharacter.h"
#include "DiabloARPGPrototype/Core/UI/YouWinWidget.h"
#include "Kismet/GameplayStatics.h"
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
    {
        HealthComponent->SetMaxHealth(500.f);
    }

    // Load the You Win widget at runtime rather than via ConstructorHelpers,
    // which is unreliable for Widget Blueprints in packaged builds.
    if (!YouWinWidgetClass)
    {
        YouWinWidgetClass = StaticLoadClass(
            UUserWidget::StaticClass(),
            nullptr,
            TEXT("/Game/UI/WBP_YouWin.WBP_YouWin_C")
        );
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        MoveComp->MaxWalkSpeed = 250.f;

    // Reconfigure perception at runtime so the boss detects the player
    // across a large room regardless of base class defaults.
    if (SightConfig)
    {
        SightConfig->SightRadius = 2000.f;
        SightConfig->LoseSightRadius = 2100.f;
        SightConfig->PeripheralVisionAngleDegrees = 180.f;

        if (PerceptionComponent)
            PerceptionComponent->ConfigureSense(*SightConfig);
    }

    // Disable leashing, the boss should always pursue once aggroed.
    LeashRadius = 99999.f;
}

void AARPGBossCharacter::OnDamaged(AActor* DamageCauser)
{
    if (!DamageCauser || CurrentState == EEnemyState::Dead)
        return;

    CurrentTarget = DamageCauser;

    // The boss skips the base class Panic/Flee threshold check entirely.
    // Any damage that does not interrupt an active Attack forces Chase.
    if (CurrentState != EEnemyState::Chase &&
        CurrentState != EEnemyState::Attack)
    {
        SetEnemyState(EEnemyState::Chase);
    }
}

void AARPGBossCharacter::HandleDeath()
{
    // Present the You Win screen before destroying the actor.
    // Creating the widget on the PlayerController ensures it persists after destruction.
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && YouWinWidgetClass)
    {
        YouWinWidgetInstance = CreateWidget<UUserWidget>(PC, YouWinWidgetClass);
        if (YouWinWidgetInstance)
        {
            YouWinWidgetInstance->AddToViewport();
            UGameplayStatics::SetGamePaused(this, true);
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeUIOnly());
        }
    }
    Super::HandleDeath();
}

void AARPGBossCharacter::EnterStagger(float Duration)
{
    // Boss is immune to stagger — do nothing
}