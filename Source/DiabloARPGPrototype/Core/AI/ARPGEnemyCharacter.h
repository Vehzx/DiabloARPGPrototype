#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionTypes.h"
#include "Components/DecalComponent.h"
#include "Components/BillboardComponent.h"
#include "ARPGEnemyCharacter.generated.h"

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
    Patrol,
    Chase,
    Attack,
    Stagger,
    Panic,
    Flee,
    Dead
};

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGEnemyCharacter();
    virtual void Tick(float DeltaTime) override;
    void FlashOnHit();
    void ApplyKnockback(const FVector& Direction, float Strength);

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    bool bIsStaggered = false;

    FTimerHandle StaggerTimerHandle;

    virtual void EnterStagger(float Duration);
    void ExitStagger();

    virtual void OnDamaged(AActor* DamageCauser);

    UPROPERTY(VisibleAnywhere, Category = "AI")
    EEnemyState CurrentState = EEnemyState::Idle;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackRange = 120.f;

    UPROPERTY()
    AActor* CurrentTarget = nullptr;

    float AttackCooldown = 1.0f;

    float TimeSinceLastAttack = 0.f;

    UPROPERTY(VisibleAnywhere, Category = "Hover")
    UDecalComponent* HoverDecal;

    AARPGEnemyAIController* GetEnemyAIController() const;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UBillboardComponent* LowHealthIcon;

    virtual bool CanEnterFlee() const { return true; }

    void ShowHoverDecal();
    void HideHoverDecal();

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackDamage = 15.f;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHealthComponent* HealthComponent;

    void SetEnemyState(EEnemyState NewState);

    virtual bool CanBeKnockedBack() const { return true; }

    virtual float GetPlayerLeashDistance() const { return LeashRadius; }

protected:
    virtual void BeginPlay() override;

    virtual bool OverridesChaseMovement() const { return false; }

    bool bStartedCombatFromPatrol = false;

    bool bIsMovingToTarget = false;

    // LEASHING SYSTEM
    FVector SpawnLocation;
    float LeashRadius = 1500.f;

    void HandleLeashReset();

    FTimerHandle LostSightTimer;
    bool bPlayerReallyLost = false;

    float TimeInChaseWithoutPath = 0.f;

    // AI Perception
    UPROPERTY(VisibleAnywhere, Category = "AI")
    UAIPerceptionComponent* PerceptionComponent;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    UFUNCTION()
    virtual void HandleDeath();

private:
    // Components
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* BodyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UWidgetComponent* HealthBarWidgetComponent;

    // AI Perception
    UPROPERTY()
    UAISenseConfig_Hearing* HearingConfig;

    UPROPERTY()
    UAISenseConfig_Damage* DamageConfig;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    void HandleStateChanged(EEnemyState OldState, EEnemyState NewState);

    // PATROL SYSTEM
    UPROPERTY(EditAnywhere, Category = "AI")
    TArray<AActor*> PatrolPoints;

    int32 CurrentPatrolIndex = 0;

    UPROPERTY(EditAnywhere, Category = "AI")
    float PatrolWaitTime = 2.0f;

    FTimerHandle PatrolWaitTimer;

    FTimerHandle PanicTimerHandle;

    // Flee direction locking
    FVector LockedFleeDirection = FVector::ZeroVector;
    float FleeDirectionLockTime = 0.f;

    FVector FleeStartLocation = FVector::ZeroVector;

    void AdvancePatrol();

    // Combat
    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackWindupTime = 0.5f;

    FTimerHandle AttackWindupTimer;

    void StartAttackWindup();
    void FinishWindupAndAttack();


    UFUNCTION()
    void PerformAttack();

    // Healing
    FTimerHandle HealOverTimeTimer;
    void HealOverTimeTick();
};