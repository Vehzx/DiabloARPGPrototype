#include "ARPGEnemyCharacter.h"
#include "Camera/CameraActor.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "DiabloARPGPrototype/Core/Player/WHealthBarWidget.h"
#include "DiabloARPGPrototype/Core/Camera/IsometricCameraPawn.h"
#include "Components/WidgetComponent.h"
#include "ARPGEnemyAIController.h"

AARPGEnemyCharacter::AARPGEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Movement Setup
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 300.f;
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }

    // Health Component
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    // Health Bar Widget Component
    HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
    HealthBarWidgetComponent->SetupAttachment(RootComponent);
    HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    HealthBarWidgetComponent->SetDrawSize(FVector2D(120.f, 12.f));
    HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

    static ConstructorHelpers::FClassFinder<UUserWidget> HealthBarClass(
        TEXT("/Game/UI/WBP_HealthBar.WBP_HealthBar_C")
    );

    if (HealthBarClass.Succeeded())
    {
        HealthBarWidgetComponent->SetWidgetClass(HealthBarClass.Class);
    }

    // AI Perception Component
    PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

    SightConfig->SightRadius = 1200.f;
    SightConfig->LoseSightRadius = 1500.f;
    SightConfig->PeripheralVisionAngleDegrees = 70.f;
    SightConfig->SetMaxAge(2.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    HearingConfig->HearingRange = 1500.f;
    HearingConfig->SetMaxAge(2.f);

    DamageConfig->SetMaxAge(2.f);

    PerceptionComponent->ConfigureSense(*SightConfig);
    PerceptionComponent->ConfigureSense(*HearingConfig);
    PerceptionComponent->ConfigureSense(*DamageConfig);
    PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

    PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AARPGEnemyCharacter::OnTargetPerceptionUpdated);

    // Visual Mesh Setup
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
    );

    if (CylinderMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderMesh.Object);
        BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
        BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -45.f));
    }

    // Apply the hit flash material
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HitFlashMat(
        TEXT("/Game/Materials/M_HitFlash.M_HitFlash")
    );

    if (HitFlashMat.Succeeded())
    {
        BodyMesh->SetMaterial(0, HitFlashMat.Object);
    }

    GetCapsuleComponent()->SetHiddenInGame(false);

    // Ensure enemy starts with correct base colour (same as player)
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
}

void AARPGEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocation = GetActorLocation();

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AARPGEnemyCharacter::HandleDeath);
    }

    if (HealthBarWidgetComponent)
    {
        if (UWHealthBarWidget* HB = Cast<UWHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
        {
            HB->InitializeHealth(HealthComponent);
        }
    }

    if (PatrolPoints.Num() > 0)
    {
        SetEnemyState(EEnemyState::Patrol);
    }
}

AARPGEnemyAIController* AARPGEnemyCharacter::GetEnemyAIController() const
{
    return Cast<AARPGEnemyAIController>(GetController());
}

void AARPGEnemyCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("Enemy died"));

    GetCharacterMovement()->DisableMovement();
    GetMesh()->SetVisibility(false, true);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Destroy();
}

void AARPGEnemyCharacter::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor)
        return;

    // Ignore camera actors and other enemies
    if (Actor->IsA(ACameraActor::StaticClass())) return;
    if (Actor->IsA(AIsometricCameraPawn::StaticClass())) return;
    if (Actor->IsA(AARPGEnemyCharacter::StaticClass())) return;

    // --- PLAYER DETECTED ---
    if (Stimulus.WasSuccessfullySensed())
    {
        CurrentTarget = Actor;

        // Only switch to Chase if not attacking or staggered
        if (CurrentState != EEnemyState::Attack &&
            CurrentState != EEnemyState::Stagger)
        {
            SetEnemyState(EEnemyState::Chase);
        }
    }
    else
    {
        // --- PLAYER LOST ---
        CurrentTarget = nullptr;

        // Heal when returning to patrol
        if (HealthComponent)
        {
            HealthComponent->ResetHealthToFull();
        }

        // If we have patrol points, return to Patrol
        if (PatrolPoints.Num() > 0)
        {
            SetEnemyState(EEnemyState::Patrol);
        }
        else
        {
            SetEnemyState(EEnemyState::Idle);
        }
    }
}

void AARPGEnemyCharacter::SetEnemyState(EEnemyState NewState)
{
    if (bIsStaggered && NewState != EEnemyState::Stagger)
        return;

    if (CurrentState == NewState)
        return;

    EEnemyState OldState = CurrentState;
    CurrentState = NewState;

    UE_LOG(LogTemp, Warning, TEXT("[AI STATE] %s -> %s"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState));

    HandleStateChanged(OldState, NewState);
}

void AARPGEnemyCharacter::HandleStateChanged(EEnemyState OldState, EEnemyState NewState)
{
    switch (NewState)
    {
    case EEnemyState::Idle:
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered IDLE state"));

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->StopMovement();
        }

        break;
    }

    case EEnemyState::Chase:
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered CHASE state"));
        break;

    case EEnemyState::Attack:
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered ATTACK state"));
        break;

    case EEnemyState::Dead:
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered DEAD state"));
        break;
    }
}

void AARPGEnemyCharacter::AdvancePatrol()
{
    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
    SetEnemyState(EEnemyState::Patrol);
}

void AARPGEnemyCharacter::PerformAttack()
{
    if (!CurrentTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("[AI] PerformAttack() called but CurrentTarget is NULL"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[AI] Attempting to damage target: %s"), *CurrentTarget->GetName());

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    if (Distance <= AttackRange)
    {
        UHealthComponent* Health = CurrentTarget->FindComponentByClass<UHealthComponent>();

        if (!Health)
        {
            UE_LOG(LogTemp, Error, TEXT("[AI] Target %s has NO HealthComponent!"), *CurrentTarget->GetName());
            return;
        }

        Health->ApplyDamage(AttackDamage, this);

        UE_LOG(LogTemp, Warning, TEXT("[AI] Enemy hit %s for %f damage"),
            *CurrentTarget->GetName(),
            AttackDamage
        );
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Target %s is out of attack range (%.1f > %.1f)"),
            *CurrentTarget->GetName(),
            Distance,
            AttackRange
        );
    }
}

void AARPGEnemyCharacter::OnDamaged(AActor* DamageCauser)
{
    if (!DamageCauser || CurrentState == EEnemyState::Dead)
        return;

    // Set target to whoever hit us
    CurrentTarget = DamageCauser;

    // If not staggered, immediately chase
    if (CurrentState != EEnemyState::Stagger)
    {
        SetEnemyState(EEnemyState::Chase);
    }
}

void AARPGEnemyCharacter::FlashOnHit()
{
    if (!BodyMesh) return;

    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.0f, 0.0f, 0.0f));

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (BodyMesh)
            {
                BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
            }
        }, 0.8f, false);
}

void AARPGEnemyCharacter::ApplyKnockback(const FVector& Direction, float Strength)
{
    LaunchCharacter(Direction * Strength, true, true);
}

void AARPGEnemyCharacter::StartAttackWindup()
{
    UE_LOG(LogTemp, Warning, TEXT("[AI] StartAttackWindup() CALLED — setting ORANGE"));

    if (BodyMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Setting BaseColour = ORANGE"));
        BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.f, 0.5f, 0.f));
    }

    GetWorldTimerManager().SetTimer(
        AttackWindupTimer,
        this,
        &AARPGEnemyCharacter::FinishWindupAndAttack,
        AttackWindupTime,
        false
    );
}

void AARPGEnemyCharacter::FinishWindupAndAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("[AI] FinishWindupAndAttack() CALLED — resetting to GREY"));

    if (BodyMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Setting BaseColour = GREY"));
        BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
    }

    PerformAttack();
}

void AARPGEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- STAGGER LOCKOUT ---
    if (bIsStaggered)
    {
        // Enemy is stunned, do nothing this frame
        return;
    }

    TimeSinceLastAttack += DeltaTime;

    // --- NAVMESH CHECK ---
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys)
    {
        FNavLocation OutLocation;
        bool bOnNav = NavSys->ProjectPointToNavigation(GetActorLocation(), OutLocation);

        if (!bOnNav)
        {
            UE_LOG(LogTemp, Error, TEXT("[AI] Enemy is NOT on the NavMesh!"));
        }
    }

    // --- PATROL LOGIC ---
    if (CurrentState == EEnemyState::Patrol)
    {
        if (PatrolPoints.Num() > 0)
        {
            AActor* PatrolTarget = PatrolPoints[CurrentPatrolIndex];
            float PatrolDistance = FVector::Dist(GetActorLocation(), PatrolTarget->GetActorLocation());

            // Move toward patrol point
            if (PatrolDistance > 150.f)
            {
                if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                {
                    AICon->MoveToActor(PatrolTarget, 5.f);
                }
            }
            else
            {
                // Reached patrol point, wait, then move to next
                GetCharacterMovement()->StopMovementImmediately();
                SetEnemyState(EEnemyState::Idle);

                GetWorldTimerManager().SetTimer(
                    PatrolWaitTimer,
                    this,
                    &AARPGEnemyCharacter::AdvancePatrol,
                    PatrolWaitTime,
                    false
                );
            }
        }

        return; // Patrol does not continue into chase/attack logic
    }

    // If no target, nothing else to do
    if (!CurrentTarget)
        return;

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    // --- CHASE LOGIC ---
    if (CurrentState == EEnemyState::Chase)
    {
        // --- LEASH CHECK ---
        float DistanceFromSpawn = FVector::Dist(GetActorLocation(), SpawnLocation);
        if (DistanceFromSpawn > LeashRadius)
        {
            HandleLeashReset();
            return;
        }

        // Switch to Attack if close enough
        if (Distance <= AttackRange)
        {
            SetEnemyState(EEnemyState::Attack);
            return;
        }

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            UE_LOG(LogTemp, Warning, TEXT("[AI] Calling MoveToActor"));

            EPathFollowingRequestResult::Type Result =
                AICon->MoveToActor(CurrentTarget, 5.f);

            switch (Result)
            {
            case EPathFollowingRequestResult::Failed:
                UE_LOG(LogTemp, Error, TEXT("[AI] MoveToActor FAILED"));
                break;

            case EPathFollowingRequestResult::AlreadyAtGoal:
                UE_LOG(LogTemp, Warning, TEXT("[AI] MoveToActor: Already at goal"));
                break;

            case EPathFollowingRequestResult::RequestSuccessful:
                UE_LOG(LogTemp, Warning, TEXT("[AI] MoveToActor: Request Successful"));
                break;
            }
        }
    }

    // --- ATTACK LOGIC ---
    if (CurrentState == EEnemyState::Attack)
    {
        // --- LEASH CHECK ---
        float DistanceFromSpawn = FVector::Dist(GetActorLocation(), SpawnLocation);
        if (DistanceFromSpawn > LeashRadius)
        {
            HandleLeashReset();
            return;
        }

        // If player moves away, go back to chase
        if (Distance > AttackRange)
        {
            SetEnemyState(EEnemyState::Chase);
            return;
        }

        // Face the player
        FVector Dir = CurrentTarget->GetActorLocation() - GetActorLocation();
        Dir.Z = 0;
        SetActorRotation(Dir.Rotation());

        // Attack if cooldown ready
        if (TimeSinceLastAttack >= AttackCooldown)
        {
            StartAttackWindup();
            TimeSinceLastAttack = 0.f;
        }
    }
}

void AARPGEnemyCharacter::HandleLeashReset()
{
    // Heal to full
    if (HealthComponent)
    {
        HealthComponent->ResetHealthToFull();
    }

    // Clear any attack windup
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    // Reset state
    CurrentState = EEnemyState::Patrol;

    // Teleport or MoveTo spawn (choose one)
    SetActorLocation(SpawnLocation);

    // Optional: reset colour
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
}

void AARPGEnemyCharacter::EnterStagger(float Duration)
{
    if (bIsStaggered)
        return;

    bIsStaggered = true;

    // Stop movement immediately
    GetCharacterMovement()->StopMovementImmediately();

    // Cancel any attack wind up or behaviour when staggered
    CurrentState = EEnemyState::Stagger;

    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    // Timer to exit stagger
    GetWorldTimerManager().SetTimer(
        StaggerTimerHandle,
        this,
        &AARPGEnemyCharacter::ExitStagger,
        Duration,
        false
    );
}

void AARPGEnemyCharacter::ExitStagger()
{
    bIsStaggered = false;

    CurrentState = EEnemyState::Chase;
}