#pragma once

#include "CoreMinimal.h"
#include "Online/LANGameModeBase.h"
#include "DrivingGameMode.generated.h"

class APlayerController;
class AController;
class ACoopVehicle;

UCLASS()
class COOP_DRIVING_API ADrivingGameMode : public ALANGameModeBase
{
    GENERATED_BODY()

public:
    ADrivingGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual void HandleStartingNewPlayer_Implementation(
        APlayerController* NewPlayer) override;

    virtual void Logout(AController* Exiting) override;

    // Called by a destination zone on the server; only the shared vehicle can finish.
    bool TryFinishDriving(ACoopVehicle* ArrivingVehicle);
    ACoopVehicle* GetSharedVehicle() const { return SharedVehicle; }

protected:
    // Fallbacks for launching a driving map directly in the editor.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
    TSoftObjectPtr<UWorld> ReturnLobbyMap;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel")
    TSoftClassPtr<class ALobbyGameMode> ReturnLobbyGameMode;

private:
    void TryAssignRoles();
    void EnsureSharedVehicle();
    void ConnectVehicle(APlayerController* Player);

    UPROPERTY(EditDefaultsOnly, Category = "Driving")
    TSubclassOf<ACoopVehicle> SharedVehicleClass;

    UPROPERTY()
    TObjectPtr<ACoopVehicle> SharedVehicle;

    TArray<TWeakObjectPtr<APlayerController>> ArrivedPlayers;

    bool bRolesAssigned = false;
    bool bReturningToLobby = false;
    FString LobbyMapPath;
    FString LobbyGameModePath;

    static constexpr int32 RequiredPlayers = 4;
};
