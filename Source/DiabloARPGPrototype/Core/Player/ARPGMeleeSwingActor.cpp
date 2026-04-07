#include "ARPGMeleeSwingActor.h"
#include "UObject/ConstructorHelpers.h"

AARPGMeleeSwingActor::AARPGMeleeSwingActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SwingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwingMesh"));
    RootComponent = SwingMesh;

    // Use a basic sphere from engine content as placeholder
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere")
    );

    if (SphereMesh.Succeeded())
    {
        SwingMesh->SetStaticMesh(SphereMesh.Object);
    }

    // Scale it to feel like a swing arc — wide and flat
    SwingMesh->SetRelativeScale3D(FVector(2.5f, 0.3f, 0.5f));

    // No collision needed — purely visual
    SwingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SwingMesh->SetReceivesDecals(false);
}

void AARPGMeleeSwingActor::BeginPlay()
{
    Super::BeginPlay();

    // Flash colour on spawn
    if (SwingMesh)
    {
        SwingMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.0f, 0.5f, 0.0f));
    }

    SetLifeSpan(LifeSpan);
}