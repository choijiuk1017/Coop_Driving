// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Online/LANGameModeBase.h"
#include "LobbyGameMode.generated.h"

class ADrivingGameMode;

/**
 * 
 */
UCLASS()
class COOP_DRIVING_API ALobbyGameMode : public ALANGameModeBase
{
	GENERATED_BODY()
	
public:
	ALobbyGameMode();

	int32 GetRequiredPlayers() const;

	bool StartGameLevel();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
	int32 RequiredPlayers = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
	FString NextLevelPath = TEXT("/Game/Maps/InGameMapTest");

    // Keep the collaborator's destination map; explicitly carry the driving rules with it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
    TSoftClassPtr<ADrivingGameMode> InGameGameMode;

private:
	bool bTravelStarted = false;
};
