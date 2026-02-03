#include "ARPGPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // ----------------------------------------------------
    // Movement Setup
    // ----------------------------------------------------
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 600.f;
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }

    // ----------------------------------------------------
    // Visual Mesh Setup
    // ----------------------------------------------------

    // Create the mesh component
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());

    // Load a simple cylinder mesh from the engine content
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

    // Ensure the capsule is visible for debugging (optional)
    GetCapsuleComponent()->SetHiddenInGame(false);
}

void AARPGPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AARPGPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}