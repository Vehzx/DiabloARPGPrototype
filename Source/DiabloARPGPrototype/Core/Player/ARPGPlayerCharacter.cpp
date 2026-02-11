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
    HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // above head

    // Load the widget class
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

        // Scale and position it to look like a humanoid placeholder
        BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.2f));
        BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -45.f));
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
                Health->ApplyDamage(25.f);
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

void AARPGPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AARPGPlayerCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("Player has died"));

    // Disable input so the player can't move after death
    DisableInput(nullptr);

    // Optional: destroy after delay
    // SetLifeSpan(3.f);
}