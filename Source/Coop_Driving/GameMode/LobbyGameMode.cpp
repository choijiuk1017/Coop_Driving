// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameMode/DrivingGameMode.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Online/LANSessionSubsystem.h"

ALobbyGameMode::ALobbyGameMode()
{
    // Keep client connections during level travel.
    bUseSeamlessTravel = true;
    InGameGameMode = TSoftClassPtr<ADrivingGameMode>(FSoftObjectPath(
        TEXT("/Game/Blueprint/BP_DrivingGameMode.BP_DrivingGameMode_C")));
}

int32 ALobbyGameMode::GetRequiredPlayers() const
{
    return RequiredPlayers;
}

bool ALobbyGameMode::StartGameLevel()
{
    if (!HasAuthority() || bTravelStarted)
    {
        return false;
    }

    if (GetNumPlayers() != RequiredPlayers)
    {
        return false;
    }

    if (!GetWorld() || NextLevelPath.IsEmpty())
    {
        return false;
    }

    UClass* DestinationGameMode = InGameGameMode.LoadSynchronous();
    if (!DestinationGameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Cannot travel: InGameGameMode is not configured or could not be loaded."));
        return false;
    }

    bTravelStarted = true;

    if (ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>())
    {
        LAN->CloseLobby();
    }

    const FString TravelURL = NextLevelPath + TEXT("?listen?game=") + DestinationGameMode->GetPathName();

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ServerTravel: %s"), *TravelURL);

    const bool bSuccess = GetWorld()->ServerTravel(TravelURL);

    if (!bSuccess)
    {
        bTravelStarted = false;
        if (ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>())
        {
            LAN->ReopenLobby();
        }
    }

    return bSuccess;
}
