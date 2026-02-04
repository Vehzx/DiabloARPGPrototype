#include "ARPGPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"

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

void AARPGPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Bind death event
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AARPGPlayerCharacter::HandleDeath);
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