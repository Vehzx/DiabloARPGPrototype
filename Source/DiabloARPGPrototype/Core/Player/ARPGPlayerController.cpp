#include "ARPGPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

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

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            Subsystem->AddMappingContext(InputMappingContext, 0);
        }
    }
}

void AARPGPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AARPGPlayerController::HandleMove);
    }
}

void AARPGPlayerController::HandleMove(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (APawn* ControlledPawn = GetPawn())
    {
        ControlledPawn->AddMovementInput(FVector::ForwardVector, MovementVector.Y);
        ControlledPawn->AddMovementInput(FVector::RightVector, MovementVector.X);
    }
}