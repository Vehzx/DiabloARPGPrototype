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
#include "Kismet/GameplayStatics.h"

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

    // Enemy Hover Decal
    HoverDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("HoverDecal"));
    HoverDecal->SetupAttachment(RootComponent);

    // Size of the decal
    HoverDecal->DecalSize = FVector(64.f, 128.f, 128.f);

    // Rotate so it projects downward
    HoverDecal->SetRelativeLocation(FVector(0.f, 0.f, -150.f));
    HoverDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

    // Start hidden
    HoverDecal->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HoverMat(
        TEXT("/Game/Materials/M_HoverRing.M_HoverRing")
    );

    if (HoverMat.Succeeded())
    {
        HoverDecal->SetDecalMaterial(HoverMat.Object);
    }

    // AI Perception Component
    PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

    SightConfig->SightRadius =  900.f;
    SightConfig->LoseSightRadius = 910.f;
    SightConfig->PeripheralVisionAngleDegrees = 60.f;
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

    // Prevent decals from projecting onto the mesh
    BodyMesh->SetReceivesDecals(false);

    // Low Health Icon
    LowHealthIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("LowHealthIcon"));
    LowHealthIcon->SetupAttachment(RootComponent);

    // Position above the head
    LowHealthIcon->SetRelativeLocation(FVector(0.f, 0.f, 180.f));

    // Start hidden
    LowHealthIcon->SetHiddenInGame(true);

    // Assign texture
    static ConstructorHelpers::FObjectFinder<UTexture2D> LowHealthTex(
        TEXT("/Game/Textures/lowhealthicon.lowhealthicon")
    );

    if (LowHealthTex.Succeeded())
    {
        LowHealthIcon->SetSprite(LowHealthTex.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
    );

    if (CylinderMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderMesh.Object);
        BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
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

    // Start in Patrol or Idle depending on editor-assigned patrol points
    if (PatrolPoints.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PATROL] Using %d patrol points assigned in editor"), PatrolPoints.Num());
        SetEnemyState(EEnemyState::Patrol);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PATROL] No patrol points assigned — starting idle"));
        SetEnemyState(EEnemyState::Idle);
    }

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
}

AARPGEnemyAIController* AARPGEnemyCharacter::GetEnemyAIController() const
{
    return Cast<AARPGEnemyAIController>(GetController());
}

void AARPGEnemyCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("Enemy died"));

    // Stop all timers to prevent callbacks on a destroyed actor
    GetWorldTimerManager().ClearAllTimersForObject(this);

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

        if (CurrentState == EEnemyState::Flee ||
            CurrentState == EEnemyState::Panic)
            return;

        // Cancel ANY pending "lost sight" logic
        GetWorldTimerManager().ClearTimer(LostSightTimer);
        bPlayerReallyLost = false;

        // Cancel patrol timer IMMEDIATELY
        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);

        CurrentTarget = Actor;

        // Only switch to Chase if not attacking or staggered
        if (CurrentState != EEnemyState::Attack &&
            CurrentState != EEnemyState::Stagger &&
            CurrentState != EEnemyState::Panic)
        {
            SetEnemyState(EEnemyState::Chase);
        }

        return;
    }

    // --- PLAYER LOST (but maybe only for a moment) ---
    GetWorldTimerManager().ClearTimer(LostSightTimer);

    TWeakObjectPtr<AARPGEnemyCharacter> WeakThis(this);

    GetWorldTimerManager().SetTimer(
        LostSightTimer,
        [WeakThis]()
        {
            if (!WeakThis.IsValid())
                return;

            if (WeakThis->CurrentState == EEnemyState::Flee)
                return;

            WeakThis->bPlayerReallyLost = true;

            AActor* Player = UGameplayStatics::GetPlayerCharacter(WeakThis.Get(), 0);

            bool bStillNoSight =
                WeakThis->PerceptionComponent &&
                !WeakThis->PerceptionComponent->HasActiveStimulus(*Player, UAISense::GetSenseID<UAISense_Sight>());

            bool bInCombat =
                WeakThis->CurrentState == EEnemyState::Chase ||
                WeakThis->CurrentState == EEnemyState::Attack ||
                WeakThis->CurrentState == EEnemyState::Stagger ||
                WeakThis->CurrentState == EEnemyState::Flee;

            if (bStillNoSight && !bInCombat)
            {
                UE_LOG(LogTemp, Warning, TEXT("[AI] Player REALLY lost — returning to patrol"));

                WeakThis->CurrentTarget = nullptr;

                if (WeakThis->HealthComponent)
                {
                    WeakThis->GetWorldTimerManager().SetTimer(
                        WeakThis->HealOverTimeTimer,
                        WeakThis.Get(),
                        &AARPGEnemyCharacter::HealOverTimeTick,
                        0.2f,
                        true
                    );
                }

                if (WeakThis->PatrolPoints.Num() > 0)
                    WeakThis->SetEnemyState(EEnemyState::Patrol);
                else
                    WeakThis->SetEnemyState(EEnemyState::Idle);
            }
            else
            {
                if (bInCombat)
                    return;
            }
        },
        0.5f,
        false
    );
}

void AARPGEnemyCharacter::SetEnemyState(EEnemyState NewState)
{
    // Allow FLEE even while staggered, but block all other transitions
    if (bIsStaggered &&
        NewState != EEnemyState::Stagger &&
        NewState != EEnemyState::Flee)
    {
        return;
    }

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
    // Cancel attack windup whenever state changes
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    // Hide low health icon
    if (OldState == EEnemyState::Panic || OldState == EEnemyState::Flee)
    {
        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(true);
    }

    switch (NewState)
    {
    case EEnemyState::Idle:
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered IDLE state"));

        // Reset movement speed after fleeing
        GetCharacterMovement()->MaxWalkSpeed = 300.f;

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->StopMovement();
        }

        break;
    }

    case EEnemyState::Chase:
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered CHASE state"));

        bIsMovingToTarget = false;

        GetWorldTimerManager().ClearTimer(HealOverTimeTimer);
        GetCharacterMovement()->MaxWalkSpeed = 400.f;

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->StopMovement();

            if (CurrentTarget)
            {
                EPathFollowingRequestResult::Type Result = AICon->MoveToActor(CurrentTarget, 5.f);
                bIsMovingToTarget = (Result == EPathFollowingRequestResult::RequestSuccessful);
            }
        }

        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
        break;
    }

    case EEnemyState::Attack:
    {
        // Stop healing when entering combat
        GetWorldTimerManager().ClearTimer(HealOverTimeTimer);

        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
        break;
    }

    case EEnemyState::Panic:
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered PANIC state"));

        // Show low health icon
        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(false);

        // Rotate away from the player immediately
        if (CurrentTarget)
        {
            FVector Away = GetActorLocation() - CurrentTarget->GetActorLocation();
            Away.Z = 0;
            SetActorRotation(Away.Rotation());
        }

        // Stop movement during panic
        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
            AICon->StopMovement();

        // Change colour to yellow/orange
        if (BodyMesh)
            BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.f, 0.7f, 0.f));

        // After a short delay, either flee OR (for ranged) go back to chase
        GetWorldTimerManager().SetTimer(
            PanicTimerHandle,
            [this]()
            {
                if (CanEnterFlee())
                {
                    SetEnemyState(EEnemyState::Flee);
                }
                else
                {
                    // Ranged enemies skip flee entirely
                    SetEnemyState(EEnemyState::Chase);
                }
            },
            0.4f, // reaction window
            false
        );

        break;
    }

    case EEnemyState::Flee:
    {
        UE_LOG(LogTemp, Warning, TEXT("[AI] Entered FLEE state"));

        FleeStartLocation = GetActorLocation();

        if (CurrentTarget)
        {
            FVector Away = GetActorLocation() - CurrentTarget->GetActorLocation();
            Away.Z = 0;
            Away.Normalize();
            LockedFleeDirection = Away;
            FleeDirectionLockTime = 1.5f;

            // Immediately move in the flee direction — no gap between StopMovement and first tick
            FVector FleeDestination = GetActorLocation() + Away * 400.f;
            if (AARPGEnemyAIController* AICon = GetEnemyAIController())
            {
                AICon->MoveToLocation(FleeDestination, 5.f);
            }
        }

        GetCharacterMovement()->MaxWalkSpeed = 600.f;
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
    UE_LOG(LogTemp, Warning, TEXT("[ONDAMAGED] State: %s | Health: %.1f"),
        *UEnum::GetValueAsString(CurrentState),
        HealthComponent ? HealthComponent->GetHealth() : -1.f);

    if (!DamageCauser || CurrentState == EEnemyState::Dead)
        return;

    CurrentTarget = DamageCauser;

    // If already fleeing, ignore damage entirely
    if (CurrentState == EEnemyState::Flee)
        return;

    // If hit during PANIC → flee immediately
    if (CurrentState == EEnemyState::Panic)
    {
        // Cancel the pending panic timer
        GetWorldTimerManager().ClearTimer(PanicTimerHandle);

        // Go straight to flee
        SetEnemyState(EEnemyState::Flee);
        return;
    }

    // Trigger PANIC at low health
    if (HealthComponent &&
        HealthComponent->GetHealth() <= HealthComponent->GetMaxHealth() * 0.25f && CanEnterFlee())
    {
        // Show low health icon
        if (LowHealthIcon)
        {
            LowHealthIcon->SetHiddenInGame(false);
        }

        // Allow panic/flee to override stagger
        bIsStaggered = false;

        if (CurrentState == EEnemyState::Stagger)
            CurrentState = EEnemyState::Idle;

        // Enter panic state
        SetEnemyState(EEnemyState::Panic);
        return;
    }

    // Otherwise chase normally
    SetEnemyState(EEnemyState::Chase);
}

void AARPGEnemyCharacter::ShowHoverDecal()
{
    if (HoverDecal)
        HoverDecal->SetVisibility(true);
}

void AARPGEnemyCharacter::HideHoverDecal()
{
    if (HoverDecal)
        HoverDecal->SetVisibility(false);
}

void AARPGEnemyCharacter::FlashOnHit()
{
    if (!BodyMesh) return;

    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.0f, 0.0f, 0.0f));

    TWeakObjectPtr<AARPGEnemyCharacter> WeakThis(this);

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [WeakThis]()
        {
            if (WeakThis.IsValid() && WeakThis->BodyMesh)
            {
                WeakThis->BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
            }
        }, 0.8f, false);
}

void AARPGEnemyCharacter::ApplyKnockback(const FVector& Direction, float Strength)
{
    LaunchCharacter(Direction * Strength, true, true);
}

void AARPGEnemyCharacter::StartAttackWindup()
{
    GetWorldTimerManager().ClearTimer(AttackWindupTimer); // Also clear windup timer here as a safety net

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
    UE_LOG(LogTemp, Warning, TEXT("[AI] FinishWindupAndAttack() CALLED – resetting to GREY"));

    if (BodyMesh)
    {
        BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
    }

    PerformAttack();

    // Only leave attack state after the attack windup ends
    SetEnemyState(EEnemyState::Chase);
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

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

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

    // --- FLEE LOGIC ---
    if (CurrentState == EEnemyState::Flee)
    {
        if (!CurrentTarget)
        {
            HandleLeashReset();
            SetEnemyState(EEnemyState::Idle);
            return;
        }

        FVector MyLocation = GetActorLocation();
        FVector PlayerLocation = CurrentTarget->GetActorLocation();

        // --- DIRECTION LOCKING ---
        if (FleeDirectionLockTime > 0.f)
        {
            FleeDirectionLockTime -= DeltaTime;

            FVector FleeDestination = MyLocation + LockedFleeDirection * 400.f;

            if (AARPGEnemyAIController* AICon = GetEnemyAIController())
            {
                AICon->MoveToLocation(FleeDestination, 5.f);
            }

            float DistanceFled = FVector::Dist2D(GetActorLocation(), FleeStartLocation);
            if (DistanceFled > 400.f)
            {
                UE_LOG(LogTemp, Warning, TEXT("[AI] Flee distance reached — resetting"));

                float DistToPlayer = CurrentTarget ?
                    FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation()) : 0.f;

                if (DistToPlayer > 400.f)
                {
                    // Player is far — full reset, heal, patrol
                    HandleLeashReset();
                    CurrentTarget = nullptr;

                    if (PatrolPoints.Num() > 0)
                        SetEnemyState(EEnemyState::Patrol);
                    else
                        SetEnemyState(EEnemyState::Idle);
                }
                else
                {
                    // Player still nearby — clear windup and go back to chase, no healing
                    GetWorldTimerManager().ClearTimer(AttackWindupTimer);
                    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
                    SetEnemyState(EEnemyState::Chase);
                }

                return;
            }

            return;
        }

        // --- NORMAL FLEE LOGIC ---
        FVector BaseAway = MyLocation - PlayerLocation;
        BaseAway.Z = 0;
        BaseAway.Normalize();

        // Try angles: 0°, 30°, -30°, 60°, -60°, 90°, -90°, 120°, -120°
        const float Angles[] = {
            0.f, 30.f, -30.f, 60.f, -60.f, 90.f, -90.f, 120.f, -120.f
        };

        FVector BestDirection = BaseAway;
        bool bFoundDirection = false;

        for (float Angle : Angles)
        {
            FRotator Rot(0.f, Angle, 0.f);
            FVector TestDir = Rot.RotateVector(BaseAway);

            FVector TestPoint = MyLocation + TestDir * 400.f;

            FNavLocation OutLoc;
            if (NavSys && NavSys->ProjectPointToNavigation(TestPoint, OutLoc))
            {
                BestDirection = TestDir;
                bFoundDirection = true;

                // LOCK THIS DIRECTION FOR STABILITY
                LockedFleeDirection = BestDirection;
                FleeDirectionLockTime = 0.25f; // quarter second lock

                break;
            }
        }

        // If no direction found, just stop fleeing and let leash reset handle it
        if (!bFoundDirection)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AI] No flee direction found — holding position"));
            return;
        }

        // Move using the best direction found
        FVector FleeDestination = MyLocation + BestDirection * 400.f;

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->MoveToLocation(FleeDestination, 5.f);
        }

        float DistanceFled = FVector::Dist2D(GetActorLocation(), FleeStartLocation);
        if (DistanceFled > 400.f)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AI] Flee distance reached — resetting"));

            float DistToPlayer = CurrentTarget ?
                FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation()) : 0.f;

            if (DistToPlayer > 400.f)
            {
                // Player is far — full reset, heal, patrol
                HandleLeashReset();
                CurrentTarget = nullptr;

                if (PatrolPoints.Num() > 0)
                    SetEnemyState(EEnemyState::Patrol);
                else
                    SetEnemyState(EEnemyState::Idle);
            }
            else
            {
                // Player still nearby — clear windup and go back to chase, no healing
                GetWorldTimerManager().ClearTimer(AttackWindupTimer);
                BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
                SetEnemyState(EEnemyState::Chase);
            }

            return;
        }

        return;
    }

    // CHASE LOGIC
    if (CurrentState == EEnemyState::Chase)
    {
        // Allow subclasses to fully override chase movement + rotation
        if (OverridesChaseMovement())
        {
            return;
        }

        // --- CHASE WATCHDOG (MELEE ONLY) ---
        {
            AARPGEnemyAIController* AICon = GetEnemyAIController();
            bool bHasPath =
                AICon &&
                AICon->GetPathFollowingComponent() &&
                AICon->GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Moving;

            if (!bHasPath)
            {
                TimeInChaseWithoutPath += DeltaTime;
            }
            else
            {
                TimeInChaseWithoutPath = 0.f;
            }

            // If we've been in CHASE for a while with no active path, re-issue MoveToActor
            if (TimeInChaseWithoutPath > 0.5f && CurrentTarget)
            {
                UE_LOG(LogTemp, Warning, TEXT("[CHASE WATCHDOG] Re-issuing MoveToActor for %s"),
                    *GetNameSafe(CurrentTarget));

                TimeInChaseWithoutPath = 0.f;

                if (AICon)
                {
                    bIsMovingToTarget = false; // force fresh request
                    AICon->StopMovement();
                    AICon->MoveToActor(CurrentTarget, 5.f);
                }
            }
        }

        // LEASH CHECK (DISTANCE FROM PLAYER)
        float DistanceToPlayer = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
        const float PlayerLeashDistance = GetPlayerLeashDistance();

        if (DistanceToPlayer > PlayerLeashDistance)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AI] Player exceeded leash distance — returning to patrol"));
            bIsMovingToTarget = false;
            CurrentTarget = nullptr;

            // Start healing if needed
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

            return;
        }

        // SWITCH TO ATTACK
        if (Distance <= AttackRange)
        {
            SetEnemyState(EEnemyState::Attack);
            return;
        }

        if (OverridesChaseMovement())
        {
            return;
        }

        // MOVE TOWARD PLAYER ONLY IF OUTSIDE ATTACK RANGE
        if (Distance > AttackRange)
        {
            // Allow subclasses to handle their own movement
            if (!OverridesChaseMovement() && !bIsMovingToTarget)
            {
                if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[BASE CHASE BEFORE] Dist=%.1f Overrides=%d MovingToTarget(before)=%d"),
                        Distance,
                        OverridesChaseMovement() ? 1 : 0,
                        bIsMovingToTarget ? 1 : 0
                    );

                    UE_LOG(LogTemp, Warning, TEXT("[AI] Calling MoveToActor"));
                    EPathFollowingRequestResult::Type Result =
                        AICon->MoveToActor(CurrentTarget, 5.f);

                    switch (Result)
                    {
                    case EPathFollowingRequestResult::Failed:
                        UE_LOG(LogTemp, Error, TEXT("[AI] MoveToActor FAILED"));
                        bIsMovingToTarget = false;
                        break;

                    case EPathFollowingRequestResult::AlreadyAtGoal:
                        UE_LOG(LogTemp, Warning, TEXT("[AI] MoveToActor: Already at goal"));
                        bIsMovingToTarget = false;
                        break;

                    case EPathFollowingRequestResult::RequestSuccessful:
                        UE_LOG(LogTemp, Warning, TEXT("[AI] MoveToActor: Request Successful"));
                        bIsMovingToTarget = true;
                        break;
                    }

                    UE_LOG(LogTemp, Warning,
                        TEXT("[BASE CHASE AFTER] MoveToActor result=%d MovingToTarget(after)=%d"),
                        (int32)Result,
                        bIsMovingToTarget ? 1 : 0
                    );
                }
            }
        }
        else
        {
            // Inside attack range → rotate toward player
            FVector Dir = CurrentTarget->GetActorLocation() - GetActorLocation();
            Dir.Z = 0;
            SetActorRotation(Dir.Rotation());
        }
    }

    // ATTACK LOGIC
    if (CurrentState == EEnemyState::Attack)
    {
        // LEASH CHECK
        float DistanceFromSpawn = FVector::Dist(GetActorLocation(), SpawnLocation);
        if (DistanceFromSpawn > LeashRadius)
        {
            HandleLeashReset();
            return;
        }

        // If player moves away, go back to chase
        if (Distance > AttackRange)
        {
            GetWorldTimerManager().ClearTimer(AttackWindupTimer); // ADD THIS

            // Reset mesh colour in case windup was in progress
            if (BodyMesh)
                BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));

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

    // Heal amount per tick
    HealthComponent->Heal(5.f);

    // Stop healing when full
    if (HealthComponent->GetHealth() >= HealthComponent->GetMaxHealth())
    {
        GetWorldTimerManager().ClearTimer(HealOverTimeTimer);
    }
}

void AARPGEnemyCharacter::HandleLeashReset()
{
    // Heal to full when leash range triggers
    if (HealthComponent)
    {
        GetWorldTimerManager().SetTimer(
            HealOverTimeTimer,
            this,
            &AARPGEnemyCharacter::HealOverTimeTick,
            0.2f,
            true    // looping
        );
    }

    // Clear any attack windup
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    // Reset mesh colour
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));

    // Hide low health icon once enemy finishes fleeing
    if (LowHealthIcon)
        LowHealthIcon->SetHiddenInGame(true);
}

void AARPGEnemyCharacter::EnterStagger(float Duration)
{
    if (bIsStaggered)
        return;

    bIsStaggered = true;

    // Cancel any pending player lost logic while staggered
    GetWorldTimerManager().ClearTimer(LostSightTimer);
    bPlayerReallyLost = false;

    GetCharacterMovement()->StopMovementImmediately();
    CurrentState = EEnemyState::Stagger;

    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

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

    // Do NOT override flee or panic when stagger ends
    if (CurrentState == EEnemyState::Flee ||
        CurrentState == EEnemyState::Panic)
        return;

    if (CurrentTarget)
    {
        SetEnemyState(EEnemyState::Chase);

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->MoveToActor(CurrentTarget, 5.f);
        }
    }
    else
    {
        CurrentState = EEnemyState::Idle;
    }
}