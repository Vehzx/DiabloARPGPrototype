#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API UGameOverWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UButton* MainMenuButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TitleText;

    UFUNCTION()
    void OnMainMenuClicked();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
    FName MainMenuLevelName = "MainMenu";
};