#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class DIABLOARPGPROTOTYPE_API AMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMainMenuGameMode();
    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditAnywhere, Category = "Menu")
    TSubclassOf<UUserWidget> MainMenuWidgetClass;

private:
    UUserWidget* MainMenuWidgetInstance;
};