#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberActor.generated.h"

class UWidgetComponent;
class UWBP_DamageNumber;

UCLASS()
class DIABLOARPGPROTOTYPE_API ADamageNumberActor : public AActor
{
    GENERATED_BODY()

public:
    void Initialize(int32 DamageAmount);

    ADamageNumberActor();

protected:
    virtual void BeginPlay() override;

private:
    int32 PendingDamage = -1;

    UPROPERTY(VisibleAnywhere)
    UWidgetComponent* WidgetComponent;

    UFUNCTION()
    void OnAnimationFinished();
};