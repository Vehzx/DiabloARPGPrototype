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
    void MoveForward(float Value);
    void MoveRight(float Value);
};