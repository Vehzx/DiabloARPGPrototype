#include "ARPGEnemyCharacter.h"
#include "Camera/CameraActor.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h" // <-- ADD THIS
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

    GetCapsuleComponent()->SetHiddenInGame(false);
}

void AARPGEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

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

    GetCharacterMovement()->DisableMovement();
    GetMesh()->SetVisibility(false, true);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Destroy();
}

void AARPGEnemyCharacter::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor)
        return;

    // Ignore camera actors
    if (Actor->IsA(ACameraActor::StaticClass()))
        return;

    // Ignore isometric camera pawn
    if (Actor->IsA(AIsometricCameraPawn::StaticClass()))
        return;

    // Ignore other enemies
    if (Actor->IsA(AARPGEnemyCharacter::StaticClass()))
        return;

    if (Stimulus.WasSuccessfullySensed())
    {
        CurrentTarget = Actor;
        SetEnemyState(EEnemyState::Chase);
    }
    else
    {
        CurrentTarget = nullptr;
        SetEnemyState(EEnemyState::Idle);
    }
}

void AARPGEnemyCharacter::SetEnemyState(EEnemyState NewState)
{
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

void AARPGEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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

    // --- CHASE LOGIC ---
    if (CurrentState == EEnemyState::Chase && CurrentTarget)
    {
        AARPGEnemyAIController* AICon = GetEnemyAIController();
        if (AICon)
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
}