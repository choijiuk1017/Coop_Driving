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

    virtual void StartPlay() override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void RestartPlayer(AController* NewPlayer) override;

    // Set on the server before entering the ready zone. Empty uses NextLevelPath.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Travel")
    void SetDrivingMap(TSoftObjectPtr<UWorld> Map);

protected:
    // Tag one PlayerStart to select the group center; otherwise use the first by name.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Spawn")
    FName SpawnCenterTag = TEXT("LobbySpawn");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Spawn", meta = (ClampMin = "1", Units = "cm"))
    float SpawnSpacing = 180.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Spawn", meta = (ClampMin = "1", Units = "cm"))
    float SpawnMaxRadius = 400.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby Spawn", meta = (ClampMin = "1", Units = "cm"))
    float SpawnClearance = 20.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
    TSoftObjectPtr<UWorld> DrivingMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
	int32 RequiredPlayers = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
	FString NextLevelPath = TEXT("/Game/Maps/InGameMapTest");

    // Use these shared driving rules for whichever map is selected.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
    TSoftClassPtr<ADrivingGameMode> InGameGameMode;

private:
    AActor* GetSpawnCenter();
    bool FindLobbySpawn(AController* Player, FTransform& Transform);
    void RetryPendingSpawns();
    TWeakObjectPtr<AActor> CachedSpawnCenter;
    TArray<TWeakObjectPtr<AController>> PendingSpawns;
    FTimerHandle SpawnRetryTimer;
    void ResetLobbyPlayer(APlayerController* Player);
	bool bTravelStarted = false;
};
