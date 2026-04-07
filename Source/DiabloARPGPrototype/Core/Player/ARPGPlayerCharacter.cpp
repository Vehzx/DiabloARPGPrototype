#include "ARPGPlayerCharacter.h"
#include "ARPGMeleeSwingActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "DiabloARPGPrototype/Core/Player/WHealthBarWidget.h"
#include "DiabloARPGPrototype/Core/UI/DamageNumberActor.h"
#include "DiabloARPGPrototype/Core/AI/ARPGEnemyCharacter.h"
#include "EngineUtils.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // ============================================================
    // Movement Setup
    // ============================================================
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 600.f;
        MoveComp->bOrientRotationToMovement = false;
        bUseControllerRotationYaw = false;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }

    // ============================================================
    // Collision
    // ============================================================
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Capsule->SetCollisionObjectType(ECC_Pawn);
    Capsule->SetCollisionResponseToAllChannels(ECR_Block);
    Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    // ============================================================
    // Health Component
    // ============================================================
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    // ============================================================
    // Health Bar Widget Component
    // ============================================================
    HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
    HealthBarWidgetComponent->SetupAttachment(RootComponent);
    HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    HealthBarWidgetComponent->SetDrawSize(FVector2D(120.f, 12.f));
    HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

    static ConstructorHelpers::FClassFinder<UUserWidget> HealthBarClass(
        TEXT("/Game/UI/WBP_HealthBar.WBP_HealthBar_C")
    );

    if (HealthBarClass.Succeeded())
    {
        HealthBarWidgetComponent->SetWidgetClass(HealthBarClass.Class);
    }

    // ============================================================
    // Visual Mesh Setup
    // ============================================================
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());

    // Load a simple cylinder mesh from Engine Content
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
    );

    if (CylinderMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderMesh.Object);
        BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
        BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
    }

    // Apply the hit flash material
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HitFlashMat(
        TEXT("/Game/Materials/M_HitFlash.M_HitFlash")
    );

    if (HitFlashMat.Succeeded())
    {
        BodyMesh->SetMaterial(0, HitFlashMat.Object);
    }

    // ============================================================
    // Camera Shake Setup
    // ============================================================
    static ConstructorHelpers::FClassFinder<UCameraShakeBase> ShakeClassFinder(
        TEXT("/Game/Camera/Shakes/BP_HitCameraShake")
    );

    if (ShakeClassFinder.Succeeded())
    {
        HitCameraShake = ShakeClassFinder.Class;
    }


    // Optional: make capsule visible for debugging
    GetCapsuleComponent()->SetHiddenInGame(false);


    // Pause Menu Setup
    static ConstructorHelpers::FClassFinder<UUserWidget> PauseMenuClass(
        TEXT("/Game/UI/WBP_PauseMenu.WBP_PauseMenu_C")
    );

    if (PauseMenuClass.Succeeded())
    {
        PauseMenuWidgetClass = PauseMenuClass.Class;
    }

    // Game Over Setup
    static ConstructorHelpers::FClassFinder<UUserWidget> GameOverClass(
        TEXT("/Game/UI/WBP_GameOver.WBP_GameOver_C")
    );

    if (GameOverClass.Succeeded())
    {
        GameOverWidgetClass = GameOverClass.Class;
        UE_LOG(LogTemp, Warning, TEXT("[PLAYER] GameOverWidgetClass loaded successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PLAYER] GameOverWidgetClass FAILED to load — check path"));
    }

    // Attack Visual
    static ConstructorHelpers::FClassFinder<AARPGMeleeSwingActor> SwingClassFinder(
        TEXT("/Game/UI/BP_MeleeSwing.BP_MeleeSwing_C")
    );

    if (SwingClassFinder.Succeeded())
    {
        MeleeSwingClass = SwingClassFinder.Class;
    }
}

void AARPGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AARPGPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AARPGPlayerCharacter::MoveRight);

    PlayerInputComponent->BindAction("TestAttack", IE_Pressed, this, &AARPGPlayerCharacter::PerformTestAttack);
    PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AARPGPlayerCharacter::Dash);

    PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &AARPGPlayerCharacter::TogglePause);
}

void AARPGPlayerCharacter::TogglePause()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    if (!bIsPaused)
    {
        // Pause
        if (PauseMenuWidgetClass)
        {
            PauseMenuWidgetInstance = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass);
            if (PauseMenuWidgetInstance)
            {
                PauseMenuWidgetInstance->AddToViewport();
                UGameplayStatics::SetGamePaused(this, true);
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
                bIsPaused = true;
            }
        }
    }
    else
    {
        // Resume
        if (PauseMenuWidgetInstance)
        {
            PauseMenuWidgetInstance->RemoveFromParent();
            PauseMenuWidgetInstance = nullptr;
        }

        UGameplayStatics::SetGamePaused(this, false);
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
        bIsPaused = false;
    }
}

FVector AARPGPlayerCharacter::GetMovementDirection() const
{
    FVector Input = GetLastMovementInputVector();
    if (!Input.IsNearlyZero())
    {
        return Input.GetSafeNormal();
    }

    // If not moving, dash in facing direction
    FVector Forward = GetActorForwardVector();
    Forward.Z = 0;
    return Forward.GetSafeNormal();
}

void AARPGPlayerCharacter::MoveForward(float Value)
{
    LastMoveDirection.X = Value; // always update, even when 0

    AddMovementInput(FVector::ForwardVector, Value);
}

void AARPGPlayerCharacter::MoveRight(float Value)
{
    LastMoveDirection.Y = Value; // always update, even when 0

    AddMovementInput(FVector::RightVector, Value);
}

void AARPGPlayerCharacter::Dash()
{
    if (bIsDashing || bDashOnCooldown)
        return;

    bIsDashing = true;
    bDashOnCooldown = true;

    FVector DashDirection;

    if (!LastMoveDirection.IsNearlyZero())
        DashDirection = FVector(LastMoveDirection.X, LastMoveDirection.Y, 0.f).GetSafeNormal();
    else
    {
        DashDirection = GetActorForwardVector();
        DashDirection.Z = 0;
        DashDirection.Normalize();
    }

    // Disable friction for smooth dash
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    MoveComp->GroundFriction = 0.f;
    MoveComp->BrakingFrictionFactor = 0.f;
    MoveComp->BrakingDecelerationWalking = 0.f;

    // Apply dash velocity
    LaunchCharacter(DashDirection * DashDistance, true, true);

    // End dash
    GetWorldTimerManager().SetTimer(
        DashTimer,
        [this]()
        {
            bIsDashing = false;

            // Restore movement friction
            UCharacterMovementComponent* MoveComp = GetCharacterMovement();
            MoveComp->GroundFriction = 8.f;
            MoveComp->BrakingFrictionFactor = 2.f;
            MoveComp->BrakingDecelerationWalking = 2048.f;
        },
        DashDuration,
        false
    );

    // Cooldown
    GetWorldTimerManager().SetTimer(
        DashCooldownTimer,
        [this]()
        {
            bDashOnCooldown = false;
        },
        DashCooldown,
        false
    );
}

void AARPGPlayerCharacter::PerformTestAttack()
{
    if (!bCanAttack)
        return;

    bCanAttack = false;

    // Rotate toward cursor (your existing logic)
    RotateTowardMouseCursor();

    // --- DEBUG: Distance to all enemies (2D only) ---
    for (TActorIterator<AARPGEnemyCharacter> It(GetWorld()); It; ++It)
    {
        float Dist2D = FVector::Dist2D(GetActorLocation(), It->GetActorLocation());
        UE_LOG(LogTemp, Warning, TEXT("[ATTACK DEBUG] Distance to %s = %f"),
            *It->GetName(), Dist2D);
    }

    // Your existing hitbox trace
    FVector Start = GetActorLocation() + FVector(0, 0, 50.f);
    FVector Forward = GetActorForwardVector();
    FVector End = Start + Forward * 200.f;

    float Radius = 60.f;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    FHitResult Hit;
    bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        End,
        FQuat::Identity,
        ECC_Pawn,
        Sphere,
        Params
    );

    // --- DEBUG: Trace result ---
    UE_LOG(LogTemp, Warning, TEXT("[ATTACK TRACE] bHit = %s"), bHit ? TEXT("TRUE") : TEXT("FALSE"));

    // Visual
    FVector SwingLocation = Start + Forward * 100.f;
    FRotator SwingRotation = GetActorRotation();
    FActorSpawnParameters SwingParams;
    SwingParams.Owner = this;

    if (MeleeSwingClass)
    {
        GetWorld()->SpawnActor<AARPGMeleeSwingActor>(
            MeleeSwingClass,
            SwingLocation,
            SwingRotation,
            SwingParams
        );
    }

    if (bHit)
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            UE_LOG(LogTemp, Warning, TEXT("[ATTACK TRACE] Hit actor: %s"), *HitActor->GetName());

            if (UHealthComponent* Health = HitActor->FindComponentByClass<UHealthComponent>())
            {
                Health->ApplyDamage(25.f, this);
                UE_LOG(LogTemp, Warning, TEXT("Hit %s for 25 damage"), *HitActor->GetName());

                // Spawn damage number at hit location
                FVector SpawnLocation = Hit.ImpactPoint;

                ADamageNumberActor* DamageActor = GetWorld()->SpawnActor<ADamageNumberActor>(
                    ADamageNumberActor::StaticClass(),
                    SpawnLocation,
                    FRotator::ZeroRotator
                );

                if (DamageActor)
                {
                    DamageActor->Initialize(25);
                }

                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                {
                    PC->PlayerCameraManager->StartCameraShake(HitCameraShake);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ATTACK TRACE] No actor hit"));
    }

    // Start cooldown
    GetWorldTimerManager().SetTimer(
        AttackCooldownTimer,
        [this]()
        {
            bCanAttack = true;
        },
        AttackCooldown,
        false
    );
}

void AARPGPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Bind death event
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AARPGPlayerCharacter::HandleDeath);
    }

    // Bind healthbar widget to health component
    if (HealthBarWidgetComponent)
    {
        if (UWHealthBarWidget* HB = Cast<UWHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
        {
            HB->InitializeHealth(HealthComponent);
        }
    }
}

void AARPGPlayerCharacter::RotateTowardMouseCursor()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
        return;

    FHitResult Hit;
    if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        FVector Target = Hit.ImpactPoint;
        FVector Direction = Target - GetActorLocation();
        Direction.Z = 0.f; // keep rotation flat

        if (!Direction.IsNearlyZero())
        {
            FRotator NewRot = Direction.Rotation();
            SetActorRotation(NewRot);
        }
    }
}

void AARPGPlayerCharacter::FlashOnHit()
{
    if (!BodyMesh) return;

    // Flash red
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.0f, 0.0f, 0.0f));

    // Revert after short delay
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (BodyMesh)
            {
                BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(0.5f, 0.5f, 0.5f));
            }
        }, 0.2f, false);
}

void AARPGPlayerCharacter::ApplyKnockback(const FVector& Direction, float Strength)
{
    LaunchCharacter(Direction * Strength, true, true);
}

void AARPGPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- DEATH SHRINK ---
    if (bIsDying)
    {
        FVector NewScale = GetActorScale3D() - FVector(DeathShrinkSpeed * DeltaTime);

        NewScale.X = FMath::Max(NewScale.X, 0.0f);
        NewScale.Y = FMath::Max(NewScale.Y, 0.0f);
        NewScale.Z = FMath::Max(NewScale.Z, 0.0f);

        SetActorScale3D(NewScale);
        return; // No need to process hover logic while dying
    }

    // --- CURSOR HOVER ENEMY DETECTION ---
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
        return;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    // Track which enemy was hovered last frame
    static AARPGEnemyCharacter* LastHoveredEnemy = nullptr;

    // Check if we are hovering an enemy
    AARPGEnemyCharacter* HoveredEnemy = Cast<AARPGEnemyCharacter>(Hit.GetActor());

    // If we hovered a NEW enemy, hide the old one
    if (LastHoveredEnemy && LastHoveredEnemy != HoveredEnemy)
    {
        LastHoveredEnemy->HideHoverDecal();
    }

    // If we are hovering an enemy, show its decal
    if (HoveredEnemy)
    {
        HoveredEnemy->ShowHoverDecal();
        LastHoveredEnemy = HoveredEnemy;
    }
    else
    {
        LastHoveredEnemy = nullptr;
    }
}

void AARPGPlayerCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("Player has died"));

    DisableInput(nullptr);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    bIsDying = true;
    SetLifeSpan(2.0f); // extend lifespan to give widget time to show

    // Show Game Over screen after short delay
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeUIOnly());
    }

    FTimerHandle GameOverTimer;
    GetWorldTimerManager().SetTimer(
        GameOverTimer,
        [this]()
        {
            APlayerController* PC = Cast<APlayerController>(GetController());
            if (PC && GameOverWidgetClass)
            {
                GameOverWidgetInstance = CreateWidget<UUserWidget>(PC, GameOverWidgetClass);
                if (GameOverWidgetInstance)
                {
                    GameOverWidgetInstance->AddToViewport();
                    UGameplayStatics::SetGamePaused(this, true);
                }
            }
        },
        0.8f,
        false
    );
}