#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "IsometricCameraPawn.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API AIsometricCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AIsometricCameraPawn();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere)
    class USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere)
    class UCameraComponent* Camera;
};