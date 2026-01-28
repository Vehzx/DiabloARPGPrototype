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

private:
    // Input assets loaded in C++
    UPROPERTY()
    UInputMappingContext* InputMappingContext;

    UPROPERTY()
    UInputAction* MoveAction;

    // Movement handler
    void HandleMove(const FInputActionValue& Value);
};