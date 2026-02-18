#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ARPGEnemyAIController.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AARPGEnemyAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;
};