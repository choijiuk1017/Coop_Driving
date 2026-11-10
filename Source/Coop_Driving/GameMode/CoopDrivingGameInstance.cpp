#include "GameMode/CoopDrivingGameInstance.h"
#include "GameMode/DrivingGameMode.h"

TSubclassOf<AGameModeBase> UCoopDrivingGameInstance::OverrideGameModeClass(
    TSubclassOf<AGameModeBase> GameModeClass, const FString& MapName,
    const FString& Options, const FString& Portal) const
{
    // TestVehicle still stores the old single-driver TestGameMod in its binary map asset.
    // Also accept the UEDPIE_<index>_ prefix used by multiplayer PIE.
    const bool bDrivingMap = MapName == TEXT("TestVehicle") || MapName.EndsWith(TEXT("_TestVehicle"));
    if (bDrivingMap && (!GameModeClass || !GameModeClass->IsChildOf(ADrivingGameMode::StaticClass())))
        return ADrivingGameMode::StaticClass();
    return Super::OverrideGameModeClass(GameModeClass, MapName, Options, Portal);
}
