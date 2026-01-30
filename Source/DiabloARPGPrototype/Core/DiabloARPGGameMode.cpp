#include "DiabloARPGGameMode.h"
#include "DiabloARPGPrototype/Core/Camera/IsometricCameraPawn.h"
#include "DiabloARPGPrototype/Core/Player/ARPGPlayerController.h"
#include "DiabloARPGPrototype/Core/Player/ARPGPlayerCharacter.h"

ADiabloARPGGameMode::ADiabloARPGGameMode()
{
	DefaultPawnClass = AARPGPlayerCharacter::StaticClass();
	PlayerControllerClass = AARPGPlayerController::StaticClass();

}