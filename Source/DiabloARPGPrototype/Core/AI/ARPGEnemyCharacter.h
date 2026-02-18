#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "ARPGEnemyCharacter.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UHealthComponent;
class UWidgetComponent;

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;

class AARPGEnemyAIController;

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
    Idle,
    Chase,
    Attack,
    Dead
};

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGEnemyCharacter();
    virtual void Tick(float DeltaTime) override;

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

    AARPGEnemyAIController* GetEnemyAIController() const;

    // Enemy State Machine
    UPROPERTY(VisibleAnywhere, Category = "AI")
    EEnemyState CurrentState = EEnemyState::Idle;

    // Current target the enemy is chasing
    UPROPERTY()
    AActor* CurrentTarget = nullptr;

    void SetEnemyState(EEnemyState NewState);
    void HandleStateChanged(EEnemyState OldState, EEnemyState NewState);
};