#include "ARPGEnemyAIController.h"
#include "GameFramework/Character.h"

AARPGEnemyAIController::AARPGEnemyAIController()
{
}

void AARPGEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    UE_LOG(LogTemp, Warning, TEXT("[AI] Controller possessed %s"), *InPawn->GetName());
    UE_LOG(LogTemp, Warning, TEXT("[AI] Controller class is: %s"), *GetClass()->GetName());
}