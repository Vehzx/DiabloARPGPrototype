#include "WHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "DiabloARPGPrototype/Core/Player/UHealthComponent.h"

void UWHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UWHealthBarWidget::InitializeHealth(UHealthComponent* InHealthComponent)
{
    HealthComponent = InHealthComponent;

    if (HealthComponent)
    {
        HealthComponent->OnHealthChanged.AddDynamic(this, &UWHealthBarWidget::OnHealthChanged);

        // Initialize bar
        OnHealthChanged(HealthComponent->GetHealth(), HealthComponent->GetMaxHealth());
    }
}

void UWHealthBarWidget::OnHealthChanged(float NewHealth, float MaxHealth)
{
    if (HealthProgressBar)
    {
        float Percent = NewHealth / MaxHealth;
        HealthProgressBar->SetPercent(Percent);
    }
}