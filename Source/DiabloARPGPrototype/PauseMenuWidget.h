#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UButton* ResumeButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* MainMenuButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TitleText;

    UFUNCTION()
    void OnResumeClicked();

    UFUNCTION()
    void OnMainMenuClicked();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
    FName MainMenuLevelName = "MainMenu";
};