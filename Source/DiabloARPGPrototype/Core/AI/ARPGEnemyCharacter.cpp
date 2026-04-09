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

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 300.f;
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }

    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

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

    // Hover decal projects downward beneath the enemy and is shown on cursor hover.
    HoverDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("HoverDecal"));
    HoverDecal->SetupAttachment(RootComponent);
    HoverDecal->DecalSize = FVector(64.f, 128.f, 128.f);
    HoverDecal->SetRelativeLocation(FVector(0.f, 0.f, -150.f));
    HoverDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
    HoverDecal->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HoverMat(
        TEXT("/Game/Materials/M_HoverRing.M_HoverRing")
    );

    if (HoverMat.Succeeded())
    {
        HoverDecal->SetDecalMaterial(HoverMat.Object);
    }

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

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());
    BodyMesh->SetReceivesDecals(false);

    // Low health icon is shown above the enemy when panic/flee threshold is reached.
    LowHealthIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("LowHealthIcon"));
    LowHealthIcon->SetupAttachment(RootComponent);
    LowHealthIcon->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
    LowHealthIcon->SetHiddenInGame(true);
 
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

    if (PatrolPoints.Num() > 0)
    {
        SetEnemyState(EEnemyState::Patrol);
    }
    else
    {
        SetEnemyState(EEnemyState::Idle);
    }

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
    // Clear all timers before destroying to prevent callbacks firing on an invalid actor.
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

    // Ignore non player actors.
    if (Actor->IsA(ACameraActor::StaticClass())) return;
    if (Actor->IsA(AIsometricCameraPawn::StaticClass())) return;
    if (Actor->IsA(AARPGEnemyCharacter::StaticClass())) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        // Flee and Panic states cannot be interrupted by perception.
        if (CurrentState == EEnemyState::Flee ||
            CurrentState == EEnemyState::Panic)
            return;

        GetWorldTimerManager().ClearTimer(LostSightTimer);
        bPlayerReallyLost = false;
        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);

        CurrentTarget = Actor;

        if (CurrentState != EEnemyState::Attack &&
            CurrentState != EEnemyState::Stagger &&
            CurrentState != EEnemyState::Panic)
        {
            SetEnemyState(EEnemyState::Chase);
        }

        return;
    }

    // Player lost, wait briefly before returning to patrol in case sight is restored.
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
    // Stagger blocks all transitions except Flee.
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

    HandleStateChanged(OldState, NewState);
}

void AARPGEnemyCharacter::HandleStateChanged(EEnemyState OldState, EEnemyState NewState)
{
    // Cancel any in progress attack windup on every state change.
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    // Hide the low health icon when leaving panic or flee.
    if (OldState == EEnemyState::Panic || OldState == EEnemyState::Flee)
    {
        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(true);
    }

    switch (NewState)
    {
    case EEnemyState::Idle:
    {
        GetCharacterMovement()->MaxWalkSpeed = 300.f;

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
        {
            AICon->StopMovement();
        }

        break;
    }

    case EEnemyState::Chase:
    {
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
        GetWorldTimerManager().ClearTimer(HealOverTimeTimer);
        GetWorldTimerManager().ClearTimer(PatrolWaitTimer);
        break;
    }

    case EEnemyState::Panic:
    {
        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(false);

        if (CurrentTarget)
        {
            FVector Away = GetActorLocation() - CurrentTarget->GetActorLocation();
            Away.Z = 0;
            SetActorRotation(Away.Rotation());
        }

        if (AARPGEnemyAIController* AICon = GetEnemyAIController())
            AICon->StopMovement();

        if (BodyMesh)
            BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.f, 0.7f, 0.f));

        // Brief reaction delay before committing to flee.
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
                    SetEnemyState(EEnemyState::Chase);
                }
            },
            0.4f,
            false
        );

        break;
    }

    case EEnemyState::Flee:
    {
        FleeStartLocation = GetActorLocation();

        // Lock the flee direction immediately to prevent the enemy briefly
        // turning back toward the player on the first tick.
        if (CurrentTarget)
        {
            FVector Away = GetActorLocation() - CurrentTarget->GetActorLocation();
            Away.Z = 0;
            Away.Normalize();
            LockedFleeDirection = Away;
            FleeDirectionLockTime = 1.5f;

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
        break;
    }
}

void AARPGEnemyCharacter::AdvancePatrol()
{
    // Do not advance patrol if the enemy is in or entering combat.
    if (CurrentState == EEnemyState::Chase ||
        CurrentState == EEnemyState::Attack ||
        CurrentState == EEnemyState::Stagger ||
        CurrentState == EEnemyState::Dead)
        return;

    if (CurrentTarget)
        return;

    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();

    if (!PatrolPoints.IsValidIndex(CurrentPatrolIndex))
        return;

    if (!CurrentTarget)
        SetEnemyState(EEnemyState::Patrol);
}

void AARPGEnemyCharacter::PerformAttack()
{
    if (!CurrentTarget)
        return;

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    if (Distance <= AttackRange)
    {
        UHealthComponent* Health = CurrentTarget->FindComponentByClass<UHealthComponent>();
        if (!Health)
            return;

        Health->ApplyDamage(AttackDamage, this);
    }
}

void AARPGEnemyCharacter::OnDamaged(AActor* DamageCauser)
{
    if (!DamageCauser || CurrentState == EEnemyState::Dead)
        return;

    CurrentTarget = DamageCauser;

    if (CurrentState == EEnemyState::Flee)
        return;

    // Taking damage while panicking skips the delay and flees immediately.
    if (CurrentState == EEnemyState::Panic)
    {
        GetWorldTimerManager().ClearTimer(PanicTimerHandle);
        SetEnemyState(EEnemyState::Flee);
        return;
    }

    // Drop below 25% health triggers panic then flee.
    if (HealthComponent &&
        HealthComponent->GetHealth() <= HealthComponent->GetMaxHealth() * 0.25f && CanEnterFlee())
    {
        if (LowHealthIcon)
            LowHealthIcon->SetHiddenInGame(false);

        bIsStaggered = false;

        if (CurrentState == EEnemyState::Stagger)
            CurrentState = EEnemyState::Idle;

        SetEnemyState(EEnemyState::Panic);
        return;
    }

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
    // Clear any previous windup before starting a new one.
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    if (BodyMesh)
        BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.f, 0.5f, 0.f));

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
    if (BodyMesh)
        BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));

    PerformAttack();
    SetEnemyState(EEnemyState::Chase);
}

void AARPGEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsStaggered)
        return;

    TimeSinceLastAttack += DeltaTime;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

    // PATROL LOGIC
    if (CurrentState == EEnemyState::Patrol)
    {
        if (PatrolPoints.Num() > 0)
        {
            AActor* PatrolTarget = PatrolPoints[CurrentPatrolIndex];
            float PatrolDistance = FVector::Dist(GetActorLocation(), PatrolTarget->GetActorLocation());

            if (PatrolDistance > 150.f)
            {
                if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                {
                    AICon->MoveToActor(PatrolTarget, 5.f);
                }
            }
            else
            {
                // Reached patrol point, wait before moving to next.
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

        return;
    }

    if (!CurrentTarget)
        return;

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    // FLEE LOGIC
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
                float DistToPlayer = CurrentTarget ?
                    FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation()) : 0.f;

                if (DistToPlayer > 400.f)
                {
                    HandleLeashReset();
                    CurrentTarget = nullptr;

                    if (PatrolPoints.Num() > 0)
                        SetEnemyState(EEnemyState::Patrol);
                    else
                        SetEnemyState(EEnemyState::Idle);
                }
                else
                {
                    // Player is still nearby, return to chase without healing.
                    GetWorldTimerManager().ClearTimer(AttackWindupTimer);
                    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
                    SetEnemyState(EEnemyState::Chase);
                }

                return;
            }

            return;
        }

        // Test wider angles away from the player to find a valid flee direction.
        FVector BaseAway = MyLocation - PlayerLocation;
        BaseAway.Z = 0;
        BaseAway.Normalize();

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
                LockedFleeDirection = BestDirection;
                FleeDirectionLockTime = 0.25f;
                break;
            }
        }

        if (!bFoundDirection)
            return;

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
                HandleLeashReset();
                CurrentTarget = nullptr;

                if (PatrolPoints.Num() > 0)
                    SetEnemyState(EEnemyState::Patrol);
                else
                    SetEnemyState(EEnemyState::Idle);
            }
            else
            {
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
        // Subclasses can manage their own movement.
        if (OverridesChaseMovement())
        {
            return;
        }

        // Watchdog: if enemy has been in Chase without an active path for
        // too long, issue MoveToActor to recover from a frozen state.
        /*
        * NOTE TO SELF - REFER TO THIS
        * http://www.atakansarioglu.com/multi-thread-watchdog-cpp/
        */
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

            if (TimeInChaseWithoutPath > 0.5f && CurrentTarget)
            {
                TimeInChaseWithoutPath = 0.f;

                if (AICon)
                {
                    bIsMovingToTarget = false;
                    AICon->StopMovement();
                    AICon->MoveToActor(CurrentTarget, 5.f);
                }
            }
        }

        // LEASH LOGIC
        float DistanceToPlayer = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
        const float PlayerLeashDistance = GetPlayerLeashDistance();

        if (DistanceToPlayer > PlayerLeashDistance)
        {
            bIsMovingToTarget = false;
            CurrentTarget = nullptr;

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

        if (Distance > AttackRange)
        {
            if (!OverridesChaseMovement() && !bIsMovingToTarget)
            {
                if (AARPGEnemyAIController* AICon = GetEnemyAIController())
                {
                    EPathFollowingRequestResult::Type Result =
                        AICon->MoveToActor(CurrentTarget, 5.f);

                    switch (Result)
                    {
                    case EPathFollowingRequestResult::Failed:
                        bIsMovingToTarget = false;
                        break;

                    case EPathFollowingRequestResult::AlreadyAtGoal:
                        bIsMovingToTarget = false;
                        break;

                    case EPathFollowingRequestResult::RequestSuccessful:
                        bIsMovingToTarget = true;
                        break;
                    }
                }
            }
        }
        else
        {
            // Inside attack range, rotate toward player
            FVector Dir = CurrentTarget->GetActorLocation() - GetActorLocation();
            Dir.Z = 0;
            SetActorRotation(Dir.Rotation());
        }
    }

    // ATTACK LOGIC
    if (CurrentState == EEnemyState::Attack)
    {
        // Leash check from spawn position
        float DistanceFromSpawn = FVector::Dist(GetActorLocation(), SpawnLocation);
        if (DistanceFromSpawn > LeashRadius)
        {
            HandleLeashReset();
            return;
        }

        // Player moved out of attack range, return to chase
        if (Distance > AttackRange)
        {
            GetWorldTimerManager().ClearTimer(AttackWindupTimer);

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

    HealthComponent->Heal(5.f);

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
            true
        );
    }

    // Clear attack windup
    GetWorldTimerManager().ClearTimer(AttackWindupTimer);

    // Reset mesh colour
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));

    // Hide low health icon
    if (LowHealthIcon)
        LowHealthIcon->SetHiddenInGame(true);
}

void AARPGEnemyCharacter::EnterStagger(float Duration)
{
    if (bIsStaggered)
        return;

    bIsStaggered = true;

    // Cancel pending lost logic
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