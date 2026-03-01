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
        MoveComp->bOrientRotationToMovement = true;
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
        BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -45.f));
    }

    // Apply the hit flash material
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HitFlashMat(
        TEXT("/Game/Materials/M_HitFlash.M_HitFlash")
    );

    if (HitFlashMat.Succeeded())
    {
        BodyMesh->SetMaterial(0, HitFlashMat.Object);
    }

    // Optional: make capsule visible for debugging
    GetCapsuleComponent()->SetHiddenInGame(false);
}

void AARPGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("TestAttack", IE_Pressed, this, &AARPGPlayerCharacter::PerformTestAttack);
}

void AARPGPlayerCharacter::PerformTestAttack()
{
    // NEW: Rotate player toward mouse cursor before attacking
    RotateTowardMouseCursor();

    FVector Start = GetActorLocation() + FVector(0, 0, 50.f); // chest height
    FVector Forward = GetActorForwardVector();
    FVector End = Start + Forward * 200.f; // Attack range

    float Radius = 60.f;

    FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); // Ignore the player

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
        AActor* HitActor = Hit.GetActor();
        if (HitActor)
        {
            UHealthComponent* Health = HitActor->FindComponentByClass<UHealthComponent>();
            if (Health)
            {
                Health->ApplyDamage(25.f, this);
                UE_LOG(LogTemp, Warning, TEXT("Hit %s for 25 damage"), *HitActor->GetName());
            }
        }
    }
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