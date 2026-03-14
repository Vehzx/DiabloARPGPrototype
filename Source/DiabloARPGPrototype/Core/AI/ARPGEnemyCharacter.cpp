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
#include "EngineUtils.h"

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
    DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

    SightConfig->SightRadius = 800.f;
    SightConfig->LoseSightRadius = 810.f;
    SightConfig->PeripheralVisionAngleDegrees = 50.f;
    SightConfig->SetMaxAge(0.2f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    DamageConfig->SetMaxAge(0.2f);

    PerceptionComponent->ConfigureSense(*SightConfig);
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
}

void AARPGEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocation = GetActorLocation();

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag("PatrolPoint"))
        {
            PatrolPoints.Add(*It);
            UE_LOG(LogTemp, Warning, TEXT("[PATROL] Added patrol point: %s"), *It->GetName());
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("PatrolPoints.Num() at runtime = %d"), PatrolPoints.Num());


    // Ensure enemy starts with correct base colour (same as player)
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));

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
        // Cancel ANY pending "lost sight" logic
        GetWorldTimerManager().ClearTimer(LostSightTimer);
        bPlayerReallyLost = false;

        // Cancel patrol timer IMMEDIATELY
        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);

        CurrentTarget = Actor;

        // Only switch to Chase if not attacking or staggered
        if (CurrentState != EEnemyState::Attack &&
            CurrentState != EEnemyState::Stagger)
        {
            SetEnemyState(EEnemyState::Chase);
        }

        return;
    }

    // --- PLAYER LOST (but maybe only for a moment) ---
    GetWorldTimerManager().ClearTimer(LostSightTimer);

    // Mark target as lost immediately
    CurrentTarget = nullptr;

    GetWorldTimerManager().SetTimer(
        LostSightTimer,
        [this]()
        {
            bPlayerReallyLost = true;

            // Only clear target if still no sight
            if (!CurrentTarget)
            {
                UE_LOG(LogTemp, Warning, TEXT("[AI] Player REALLY lost — returning to patrol"));

                // Heal when returning to patrol
                if (HealthComponent)
                {
                    GetWorldTimerManager().SetTimer(
                        HealOverTimeTimer,
                        this,
                        &AARPGEnemyCharacter::HealOverTimeTick,
                        0.2f,
                        true
                    );
                }

                if (PatrolPoints.Num() > 0)
                {
                    SetEnemyState(EEnemyState::Patrol);
                }
                else
                {
                    SetEnemyState(EEnemyState::Idle);
                }
            }
        },
        0.5f,   // half-second grace period
        false
    );
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
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered CHASE state"));

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->StopMovement();
        }

        // Cancel any pending patrol timer
        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);

        break;
    }

    case EEnemyState::Attack:
    {
        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
        break;
    }

    case EEnemyState::Dead:
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered DEAD state"));
        break;
    }
}

void AARPGEnemyCharacter::AdvancePatrol()
{
    // Abort AdvancePatrol ONLY if in combat states
    if (CurrentState == EEnemyState::Chase ||
        CurrentState == EEnemyState::Attack ||
        CurrentState == EEnemyState::Stagger ||
        CurrentState == EEnemyState::Dead)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PATROL] AdvancePatrol ABORTED — AI in combat state (%s)"),
            *UEnum::GetValueAsString(CurrentState));
        return;
    }

    // Never advance patrol if we currently have a target
    if (CurrentTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PATROL] AdvancePatrol BLOCKED because CurrentTarget is valid (%s)"),
            *GetNameSafe(CurrentTarget));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[PATROL] Advancing patrol point. Old index: %d"), CurrentPatrolIndex);

    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();

    UE_LOG(LogTemp, Warning, TEXT("[PATROL] New patrol index: %d"), CurrentPatrolIndex);

    if (PatrolPoints.IsValidIndex(CurrentPatrolIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("[PATROL] New patrol target: %s"),
            *GetNameSafe(PatrolPoints[CurrentPatrolIndex]));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PATROL] INVALID patrol index after advance!"));
        return;
    }

    // We should only enter Patrol if we still have no target
    if (!CurrentTarget)
    {
        SetEnemyState(EEnemyState::Patrol);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PATROL] Patrol state NOT set because target exists"));
    }
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
    UE_LOG(LogTemp, Warning, TEXT("[AI] StartAttackWindup() CALLED � setting ORANGE"));

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
    UE_LOG(LogTemp, Warning, TEXT("[AI] FinishWindupAndAttack() CALLED � resetting to GREY"));

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

void AARPGEnemyCharacter::HealOverTimeTick()
{
    if (!HealthComponent) return;

    // Heal amount per tick (tweak as needed)
    HealthComponent->Heal(5.f);

    // Stop healing when full
    if (HealthComponent->GetHealth() >= HealthComponent->GetMaxHealth())
    {
        GetWorldTimerManager().ClearTimer(HealOverTimeTimer);
    }
}

void AARPGEnemyCharacter::HandleLeashReset()
{
    // Heal to full
    if (HealthComponent)
    {
        GetWorldTimerManager().SetTimer(
            HealOverTimeTimer,
            this,
            &AARPGEnemyCharacter::HealOverTimeTick,
            0.2f,   // tick rate
            true    // looping
        );
    }

    // Clear any attack windup
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

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