#include "ARPGPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "DiabloARPGPrototype/Core/Player/WHealthBarWidget.h"

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
}

void AARPGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AARPGPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AARPGPlayerCharacter::MoveRight);

    PlayerInputComponent->BindAction("TestAttack", IE_Pressed, this, &AARPGPlayerCharacter::PerformTestAttack);
    PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AARPGPlayerCharacter::Dash);
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
    UE_LOG(LogTemp, Warning, TEXT("[MOVE] MoveForward Value: %f"), Value);

    LastMoveDirection.X = Value; // always update, even when 0

    AddMovementInput(FVector::ForwardVector, Value);
}

void AARPGPlayerCharacter::MoveRight(float Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[MOVE] MoveRight Value: %f"), Value);

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

    // Debug
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 2.f);
    DrawDebugSphere(GetWorld(), End, Radius, 12, FColor::Red, false, 1.f);

    if (bHit)
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            if (UHealthComponent* Health = HitActor->FindComponentByClass<UHealthComponent>())
            {
                Health->ApplyDamage(25.f, this);
                UE_LOG(LogTemp, Warning, TEXT("Hit %s for 25 damage"), *HitActor->GetName());

                if (APlayerController* PC = Cast<APlayerController>(GetController()))
                {
                    PC->PlayerCameraManager->StartCameraShake(HitCameraShake);
                }
            }
        }
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

    if (bIsDying)
    {
        FVector NewScale = GetActorScale3D() - FVector(DeathShrinkSpeed * DeltaTime);

        // Clamp to zero so it never goes negative
        NewScale.X = FMath::Max(NewScale.X, 0.0f);
        NewScale.Y = FMath::Max(NewScale.Y, 0.0f);
        NewScale.Z = FMath::Max(NewScale.Z, 0.0f);

        SetActorScale3D(NewScale);
    }
}

void AARPGPlayerCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("Player has died"));

    // Disable input
    DisableInput(nullptr);

    // Disable movement
    GetCharacterMovement()->DisableMovement();

    // Disable collision
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Begin shrinking
    bIsDying = true;

    // Destroy after 1 second
    SetLifeSpan(1.0f);
}