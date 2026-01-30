#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "ARPGPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AARPGPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    UPROPERTY()
    UInputMappingContext* InputMappingContext;

    UPROPERTY()
    UInputAction* MoveAction;

    void HandleMove(const FInputActionValue& Value);
};