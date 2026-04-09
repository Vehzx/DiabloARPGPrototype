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

    // Movement
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 600.f;
        MoveComp->bOrientRotationToMovement = false;
        bUseControllerRotationYaw = false;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }

    // Collision
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Capsule->SetCollisionObjectType(ECC_Pawn);
    Capsule->SetCollisionResponseToAllChannels(ECR_Block);
    Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    // Health
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    // Health Bar Widget
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

    // Visual Mesh
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
    );

    if (CylinderMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderMesh.Object);
        BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
        BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HitFlashMat(
        TEXT("/Game/Materials/M_HitFlash.M_HitFlash")
    );

    if (HitFlashMat.Succeeded())
    {
        BodyMesh->SetMaterial(0, HitFlashMat.Object);
    }

    // Camera Shake
    static ConstructorHelpers::FClassFinder<UCameraShakeBase> ShakeClassFinder(
        TEXT("/Game/Camera/Shakes/BP_HitCameraShake")
    );

    if (ShakeClassFinder.Succeeded())
    {
        HitCameraShake = ShakeClassFinder.Class;
    }

    GetCapsuleComponent()->SetHiddenInGame(false);

    // Pause Menu
    static ConstructorHelpers::FClassFinder<UUserWidget> PauseMenuClass(
        TEXT("/Game/UI/WBP_PauseMenu.WBP_PauseMenu_C")
    );

    if (PauseMenuClass.Succeeded())
    {
        PauseMenuWidgetClass = PauseMenuClass.Class;
    }

    // Game Over
    static ConstructorHelpers::FClassFinder<UUserWidget> GameOverClass(
        TEXT("/Game/UI/WBP_GameOver.WBP_GameOver_C")
    );

    if (GameOverClass.Succeeded())
    {
        GameOverWidgetClass = GameOverClass.Class;
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
        // Enter pause state
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
        // Exit pause state
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
    // Prefer actual movement input
    FVector Input = GetLastMovementInputVector();
    if (!Input.IsNearlyZero())
    {
        return Input.GetSafeNormal();
    }

    // If stationary, dash in facing direction
    FVector Forward = GetActorForwardVector();
    Forward.Z = 0;
    return Forward.GetSafeNormal();
}

void AARPGPlayerCharacter::MoveForward(float Value)
{
    LastMoveDirection.X = Value;

    AddMovementInput(FVector::ForwardVector, Value);
}

void AARPGPlayerCharacter::MoveRight(float Value)
{
    LastMoveDirection.Y = Value;

    AddMovementInput(FVector::RightVector, Value);
}

void AARPGPlayerCharacter::Dash()
{
    if (bIsDashing || bDashOnCooldown)
        return;

    bIsDashing = true;
    bDashOnCooldown = true;

    // Determine dash direction (movement input or facing direction)
    FVector DashDirection;

    if (!LastMoveDirection.IsNearlyZero())
        DashDirection = FVector(LastMoveDirection.X, LastMoveDirection.Y, 0.f).GetSafeNormal();
    else
    {
        DashDirection = GetActorForwardVector();
        DashDirection.Z = 0;
        DashDirection.Normalize();
    }

    // Temporarily remove friction for a smooth dash
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    MoveComp->GroundFriction = 0.f;
    MoveComp->BrakingFrictionFactor = 0.f;
    MoveComp->BrakingDecelerationWalking = 0.f;

    LaunchCharacter(DashDirection * DashDistance, true, true);

    // Restore movement friction after dash ends
    GetWorldTimerManager().SetTimer(
        DashTimer,
        [this]()
        {
            bIsDashing = false;

            UCharacterMovementComponent* MoveComp = GetCharacterMovement();
            MoveComp->GroundFriction = 8.f;
            MoveComp->BrakingFrictionFactor = 2.f;
            MoveComp->BrakingDecelerationWalking = 2048.f;
        },
        DashDuration,
        false
    );

    // Dash cooldown
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

    // Face the cursor before attacking
    RotateTowardMouseCursor();

    // Sweep parameters
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

    // Spawn swing visual
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

    // Apply damage + spawn damage numbers
    if (bHit)
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            if (UHealthComponent* Health = HitActor->FindComponentByClass<UHealthComponent>())
            {
                Health->ApplyDamage(25.f, this);

                // Damage number popup
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

                // Camera shake feedback
                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                {
                    PC->PlayerCameraManager->StartCameraShake(HitCameraShake);
                }
            }
        }
    }

    // Attack cooldown
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

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AARPGPlayerCharacter::HandleDeath);
    }

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
        Direction.Z = 0.f;

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

    // Flash briefly
    BodyMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.0f, 0.0f, 0.0f));

    TWeakObjectPtr<AARPGPlayerCharacter> WeakThis(this);

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(
        TimerHandle,
        [WeakThis]()
        {
            if (!WeakThis.IsValid()) return;

            if (WeakThis->BodyMesh)
            {
                WeakThis->BodyMesh->SetVectorParameterValueOnMaterials(
                    "BaseColour",
                    FVector(0.5f, 0.5f, 0.5f)
                );
            }
        },
        0.2f,
        false
    );
}

void AARPGPlayerCharacter::ApplyKnockback(const FVector& Direction, float Strength)
{
    LaunchCharacter(Direction * Strength, true, true);
}

void AARPGPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Shrink effect during death animation
    if (bIsDying)
    {
        FVector NewScale = GetActorScale3D() - FVector(DeathShrinkSpeed * DeltaTime);

        NewScale.X = FMath::Max(NewScale.X, 0.0f);
        NewScale.Y = FMath::Max(NewScale.Y, 0.0f);
        NewScale.Z = FMath::Max(NewScale.Z, 0.0f);

        SetActorScale3D(NewScale);
        return;
    }

    // Cursor hover detection for enemies
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
        return;

    FHitResult Hit;
    PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    AARPGEnemyCharacter* HoveredEnemy = Cast<AARPGEnemyCharacter>(Hit.GetActor());

    // Hide previous hover decal
    if (LastHoveredEnemy && LastHoveredEnemy != HoveredEnemy)
    {
        LastHoveredEnemy->HideHoverDecal();
    }

    // Show new hover decal
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

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        DisableInput(PC);
    }

    if (GetCharacterMovement())
        GetCharacterMovement()->DisableMovement();

    if (GetCapsuleComponent())
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bIsDying = true;

    SetLifeSpan(2.0f);

    // Prepare UI for game over
    if (PC && GameOverWidgetClass)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeUIOnly());
    }

    // Delay before showing game over widget
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