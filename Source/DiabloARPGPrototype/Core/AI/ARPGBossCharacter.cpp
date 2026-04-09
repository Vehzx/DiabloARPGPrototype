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
    {
        HealthComponent->SetMaxHealth(500.f);
        UE_LOG(LogTemp, Warning, TEXT("[BOSS] OnDeath delegate bound successfully"));
    }

    // Load YouWin widget class at runtime
    if (!YouWinWidgetClass)
    {
        YouWinWidgetClass = StaticLoadClass(
            UUserWidget::StaticClass(),
            nullptr,
            TEXT("/Game/UI/WBP_YouWin.WBP_YouWin_C")
        );

        if (YouWinWidgetClass)
            UE_LOG(LogTemp, Warning, TEXT("[BOSS] YouWinWidgetClass loaded successfully"))
        else
            UE_LOG(LogTemp, Error, TEXT("[BOSS] YouWinWidgetClass FAILED to load"));
    }

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

void AARPGBossCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("[BOSS] Boss defeated — showing You Win screen"));

    // Show You Win widget before calling Super which destroys the actor
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

    // Call base class death handling after widget is shown
    Super::HandleDeath();
}

void AARPGBossCharacter::EnterStagger(float Duration)
{
    // Boss is immune to stagger — do nothing
    UE_LOG(LogTemp, Warning, TEXT("[BOSS] Stagger ignored — boss is immune"));
}