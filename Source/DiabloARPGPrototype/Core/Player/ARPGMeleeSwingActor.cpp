#include "ARPGMeleeSwingActor.h"
#include "UObject/ConstructorHelpers.h"

AARPGMeleeSwingActor::AARPGMeleeSwingActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SwingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwingMesh"));
    RootComponent = SwingMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere")
    );

    if (SphereMesh.Succeeded())
    {
        SwingMesh->SetStaticMesh(SphereMesh.Object);
    }

    SwingMesh->SetRelativeScale3D(FVector(2.5f, 0.3f, 0.5f));
    SwingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SwingMesh->SetReceivesDecals(false);
}

void AARPGMeleeSwingActor::BeginPlay()
{
    Super::BeginPlay();

    if (SwingMesh)
    {
        SwingMesh->SetVectorParameterValueOnMaterials("BaseColour", FVector(1.0f, 0.5f, 0.0f));
    }

    SetLifeSpan(LifeSpan);
}