#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGPlayerCharacter.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UHealthComponent;
class UWidgetComponent;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGPlayerCharacter();

protected:
    virtual void BeginPlay() override;

    /** Input bindings */
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
    virtual void Tick(float DeltaTime) override;

private:

    // ============================================================
    // Components
    // ============================================================

    /** Simple placeholder mesh for the character body */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* BodyMesh;

    /** Health system for the player */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UHealthComponent* HealthComponent;

    /** Floating healthbar widget */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UWidgetComponent* HealthBarWidgetComponent;

    // ============================================================
    // Internal Functions
    // ============================================================

    /** Called when the player dies */
    UFUNCTION()
    void HandleDeath();

    /** Test attack for debugging (left-click) */
    UFUNCTION()
    void PerformTestAttack();

    void RotateTowardMouseCursor();
};