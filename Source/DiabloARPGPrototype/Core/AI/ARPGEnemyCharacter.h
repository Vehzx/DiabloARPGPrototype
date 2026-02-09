#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGEnemyCharacter.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UHealthComponent;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGEnemyCharacter();

protected:
    virtual void BeginPlay() override;

private:

    // ============================================================
    // Components
    // ============================================================

    /** Simple placeholder mesh for the enemy body */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* BodyMesh;

    /** Health system for the enemy */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHealthComponent* HealthComponent;

    // ============================================================
    // Internal Functions
    // ============================================================

    /** Called when the enemy dies */
    UFUNCTION()
    void HandleDeath();
};