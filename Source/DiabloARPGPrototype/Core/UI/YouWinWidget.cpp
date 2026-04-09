#include "YouWinWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UYouWinWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (MainMenuButton)
        MainMenuButton->OnClicked.AddDynamic(this, &UYouWinWidget::OnMainMenuClicked);

    if (TitleText)
        TitleText->SetText(FText::FromString(TEXT("You Win!")));
}

void UYouWinWidget::OnMainMenuClicked()
{
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}