#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGMeleeSwingActor.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGMeleeSwingActor : public AActor
{
    GENERATED_BODY()

public:
    AARPGMeleeSwingActor();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* SwingMesh;

    UPROPERTY(EditAnywhere, Category = "Swing")
    float LifeSpan = 0.15f;
};