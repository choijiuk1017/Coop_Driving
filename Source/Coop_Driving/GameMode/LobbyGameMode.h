// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class COOP_DRIVING_API ALobbyGameMode : public AGameModeBase
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
	FString NextLevelPath = TEXT("/Game/Maps/GameMap");

private:
	bool bTravelStarted = false;
};
