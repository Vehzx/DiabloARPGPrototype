#include "PauseMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "DiabloARPGPrototype/Core/Player/ARPGPlayerCharacter.h"

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ResumeButton)
        ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);

    if (MainMenuButton)
        MainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);
}

void UPauseMenuWidget::OnResumeClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn()))
    {
        Player->TogglePause();

        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
}


void UPauseMenuWidget::OnMainMenuClicked()
{
    // Unpause before travelling to avoid carrying paused state
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}