#include "IsometricCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AIsometricCameraPawn::AIsometricCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    RootComponent = SpringArm;

    SpringArm->TargetArmLength = 1200.f;
    SpringArm->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);
}

void AIsometricCameraPawn::BeginPlay()
{
    Super::BeginPlay();
}

void AIsometricCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AIsometricCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}