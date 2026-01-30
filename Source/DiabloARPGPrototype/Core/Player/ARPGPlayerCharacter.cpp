#include "ARPGPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AARPGPlayerCharacter::AARPGPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    UE_LOG(LogTemp, Warning, TEXT("[CHAR] Constructor fired"));

    if (GetCapsuleComponent())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CHAR] CapsuleComponent OK"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[CHAR] CapsuleComponent MISSING"));
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CHAR] MovementComponent OK"));
        MoveComp->MaxWalkSpeed = 600.f;
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[CHAR] MovementComponent MISSING"));
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