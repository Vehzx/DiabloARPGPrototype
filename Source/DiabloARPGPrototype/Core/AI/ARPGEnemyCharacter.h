#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGEnemyCharacter.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UHealthComponent;
class UWidgetComponent;

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGEnemyCharacter();

protected:
    virtual void BeginPlay() override;

private:

    // Components
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* BodyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHealthComponent* HealthComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UWidgetComponent* HealthBarWidgetComponent;

    UFUNCTION()
    void HandleDeath();

    // AI Perception
    UPROPERTY(VisibleAnywhere, Category = "AI")
    UAIPerceptionComponent* PerceptionComponent;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    UPROPERTY()
    UAISenseConfig_Hearing* HearingConfig;

    UPROPERTY()
    UAISenseConfig_Damage* DamageConfig;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};