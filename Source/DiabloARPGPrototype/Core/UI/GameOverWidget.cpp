#include "GameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (MainMenuButton)
        MainMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnMainMenuClicked);

    if (TitleText)
        TitleText->SetText(FText::FromString(TEXT("Game Over")));
}

void UGameOverWidget::OnMainMenuClicked()
{
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}