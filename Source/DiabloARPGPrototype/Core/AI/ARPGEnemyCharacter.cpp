#include "ARPGEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"
#include "DiabloARPGPrototype/Core/Player/WHealthBarWidget.h"

AARPGEnemyCharacter::AARPGEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    // ============================================================
    // Movement Setup
    // ============================================================
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 300.f;
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

        UE_LOG(LogTemp, Warning, TEXT("Enemy BodyMesh: %s"),
            BodyMesh->GetStaticMesh() ? TEXT("Loaded") : TEXT("NULL"));

    }

    // Optional: make capsule visible for debugging
    GetCapsuleComponent()->SetHiddenInGame(false);
}

void AARPGEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AARPGEnemyCharacter::HandleDeath);
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

void AARPGEnemyCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("Enemy died"));

    // Stop movement
    GetCharacterMovement()->DisableMovement();

    // Hide the mesh + capsule
    GetMesh()->SetVisibility(false, true);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Destroy after delay
    Destroy();
}
