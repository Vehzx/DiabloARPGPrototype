#include "ARPGPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = 600.f;
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }
}

void AARPGPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AARPGPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}