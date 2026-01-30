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
}; 