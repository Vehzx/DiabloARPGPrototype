#include "ARPGPlayerController.h"
#include "EngineUtils.h"
#include "DiabloARPGPrototype/Core/Camera/IsometricCameraPawn.h"
#include "DiabloARPGPrototype/Core/Player/ARPGPlayerCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"

AARPGPlayerController::AARPGPlayerController()
{
    // Load Input Mapping Context
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_PlayerObj(
        TEXT("/Game/Input/IMC_Player.IMC_Player")
    );
    if (IMC_PlayerObj.Succeeded())
    {
        InputMappingContext = IMC_PlayerObj.Object;
    }

    // Load Move Input Action
    static ConstructorHelpers::FObjectFinder<UInputAction> IA_MoveObj(
        TEXT("/Game/Input/IA_Move.IA_Move")
    );
    if (IA_MoveObj.Succeeded())
    {
        MoveAction = IA_MoveObj.Object;
    }
}

void AARPGPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[PC] BeginPlay fired"));

    if (GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PC] Possessed Pawn at BeginPlay: %s"), *GetPawn()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PC] No pawn possessed at BeginPlay"));
    }

    // Find camera pawn and hook it up
    AIsometricCameraPawn* Cam = nullptr;
    for (TActorIterator<AIsometricCameraPawn> It(GetWorld()); It; ++It)
    {
        Cam = *It;
        break;
    }

    if (Cam)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PC] Camera Pawn found: %s"), *Cam->GetName());
        Cam->SetFollowTarget(GetPawn());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PC] No Camera Pawn found in level"));
    }
}

void AARPGPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    UE_LOG(LogTemp, Warning, TEXT("[PC] OnPossess fired. Pawn: %s"), *InPawn->GetName());

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            if (InputMappingContext)
            {
                Subsystem->AddMappingContext(InputMappingContext, 0);
                UE_LOG(LogTemp, Warning, TEXT("[PC] Mapping Context added in OnPossess"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[PC] InputMappingContext is NULL in OnPossess"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[PC] EnhancedInput Subsystem NOT found in OnPossess"));
        }
    }
}

void AARPGPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UE_LOG(LogTemp, Warning, TEXT("[PC] SetupInputComponent fired"));

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("[PC] EnhancedInputComponent found"));

        if (MoveAction)
        {
            UE_LOG(LogTemp, Warning, TEXT("[PC] MoveAction is valid, binding now"));
            EnhancedInput->BindAction(
                MoveAction,
                ETriggerEvent::Triggered,
                this,
                &AARPGPlayerController::HandleMove
            );
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[PC] MoveAction is NULL"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PC] EnhancedInputComponent NOT found"));
    }
}

void AARPGPlayerController::HandleMove(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    UE_LOG(LogTemp, Warning, TEXT("[PC] HandleMove fired: %s"), *MovementVector.ToString());
    UE_LOG(LogTemp, Warning, TEXT("[PC] Raw Move Vector: X=%f Y=%f"), MovementVector.X, MovementVector.Y);

    if (APawn* ControlledPawn = GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PC] Controlled Pawn: %s"), *ControlledPawn->GetName());

        ControlledPawn->AddMovementInput(FVector::ForwardVector, MovementVector.Y);
        ControlledPawn->AddMovementInput(FVector::RightVector, MovementVector.X);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PC] No pawn possessed during movement"));
    }
}