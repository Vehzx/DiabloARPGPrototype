#include "ARPGPlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "DiabloARPGPrototype/Core/Camera/IsometricCameraPawn.h"

AARPGPlayerController::AARPGPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AARPGPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    // Attach camera pawn to player
    for (TActorIterator<AIsometricCameraPawn> It(GetWorld()); It; ++It)
    {
        AIsometricCameraPawn* Cam = *It;
        Cam->SetFollowTarget(GetPawn());
        break;
    }
}

void AARPGPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAxis("MoveForward", this, &AARPGPlayerController::MoveForward);
    InputComponent->BindAxis("MoveRight", this, &AARPGPlayerController::MoveRight);
}

void AARPGPlayerController::MoveForward(float Value)
{
    if (APawn* P = GetPawn())
    {
        P->AddMovementInput(P->GetActorForwardVector(), Value);
    }
}

void AARPGPlayerController::MoveRight(float Value)
{
    if (APawn* P = GetPawn())
    {
        P->AddMovementInput(P->GetActorRightVector(), Value);
    }
}