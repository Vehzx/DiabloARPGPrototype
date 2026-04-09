#include "DamageNumberActor.h"
#include "Components/WidgetComponent.h"
#include "WBP_DamageNumber.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Animation/UMGSequencePlayer.h"
#include "Animation/WidgetAnimation.h"

ADamageNumberActor::ADamageNumberActor()
{
    PrimaryActorTick.bCanEverTick = false;

    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    RootComponent = WidgetComponent;

    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(FVector2D(100.f, 50.f));
    WidgetComponent->SetTwoSided(true);

    static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/UI/WBP_DamageNumber"));
    if (WidgetClassFinder.Succeeded())
    {
        WidgetComponent->SetWidgetClass(WidgetClassFinder.Class);
    }
}

void ADamageNumberActor::BeginPlay()
{
    Super::BeginPlay();

    // Face the camera on spawn
    if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        FVector CamLoc = Cam->GetCameraLocation();
        FVector ToCam = CamLoc - GetActorLocation();
        ToCam.Z = 0.f;
        SetActorRotation(ToCam.Rotation());
    }

    WidgetComponent->InitWidget();

    // Apply pending damage once widget exists
    if (PendingDamage >= 0)
    {
        Initialize(PendingDamage);
    }
}

void ADamageNumberActor::Initialize(int32 DamageAmount)
{
    PendingDamage = DamageAmount;

    if (UWBP_DamageNumber* Widget = Cast<UWBP_DamageNumber>(WidgetComponent->GetUserWidgetObject()))
    {
        Widget->SetDamageValue(DamageAmount);

        if (UWidgetAnimation* Anim = Widget->GetFloatUpAnimation())
        {
            Widget->PlayAnimation(Anim);

            float Duration = Anim->GetEndTime();

            FTimerHandle Timer;
            GetWorldTimerManager().SetTimer(
                Timer,
                this,
                &ADamageNumberActor::OnAnimationFinished,
                Duration,
                false
            );
        }
    }
}

void ADamageNumberActor::OnAnimationFinished()
{
    Destroy();
}