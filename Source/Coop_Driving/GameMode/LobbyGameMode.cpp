// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Online/LANSessionSubsystem.h"

ALobbyGameMode::ALobbyGameMode()
{
    // Keep client connections during level travel.
    bUseSeamlessTravel = true;
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

    bTravelStarted = true;

    if (ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>())
    {
        LAN->CloseLobby();
    }

    const FString TravelURL = NextLevelPath + TEXT("?listen");

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
