#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ARPGPlayerCharacter.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UHealthComponent;
class UWidgetComponent;
class UCameraShakeBase;

UCLASS()
class DIABLOARPGPROTOTYPE_API AARPGPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AARPGPlayerCharacter();
    void FlashOnHit();

    virtual void Tick(float DeltaTime) override;
    void ApplyKnockback(const FVector& Direction, float Strength);

    void TogglePause();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    /** Input bindings */
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /* Movement */
    void MoveForward(float Value);
    void MoveRight(float Value);

    FVector GetMovementDirection() const;

    FVector LastMoveDirection = FVector::ZeroVector;

    void Dash();

    UPROPERTY(EditAnywhere, Category = "Movement")
    float DashDistance = 1200.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float DashDuration = 0.12f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float DashCooldown = 0.5f;

    bool bIsDashing = false;
    bool bDashOnCooldown = false;

    FTimerHandle DashTimer;
    FTimerHandle DashCooldownTimer;

    UPROPERTY(EditAnywhere, Category = "Camera")
    TSubclassOf<UCameraShakeBase> HitCameraShake;

private:
    UUserWidget* PauseMenuWidgetInstance;
    bool bIsPaused = false;

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

    /* Called when the player dies */
    UFUNCTION()
    void HandleDeath();

    /** Test attack for debugging (left-click) */
    UFUNCTION()
    void PerformTestAttack();

    void RotateTowardMouseCursor();

    bool bIsDying = false;

    float DeathShrinkSpeed = 1.5f;

    // ============================================================
    // Combat
    // ============================================================

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackCooldown = 0.8f;

    bool bCanAttack = true;

    FTimerHandle AttackCooldownTimer;

};