#include "DiabloARPGGameMode.h"
#include "DiabloARPGPrototype/Core/Camera/IsometricCameraPawn.h"
#include "DiabloARPGPrototype/Core/Player/ARPGPlayerController.h"

ADiabloARPGGameMode::ADiabloARPGGameMode()
{
    DefaultPawnClass = AIsometricCameraPawn::StaticClass();
    PlayerControllerClass = AARPGPlayerController::StaticClass();
}