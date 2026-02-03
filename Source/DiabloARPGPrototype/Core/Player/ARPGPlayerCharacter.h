#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGPlayerCharacter.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGPlayerCharacter();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

private:

    // ----------------------------------------------------
    // Components
    // ----------------------------------------------------

    /** Simple placeholder mesh for the character body */
    UPROPERTY(VisibleAnywhere, Category = "Visual")
    class UStaticMeshComponent* BodyMesh;
};