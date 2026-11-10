#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DrivingGameMode.generated.h"

class APlayerController;
class AController;
class ACoopVehicle;

UCLASS()
class COOP_DRIVING_API ADrivingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADrivingGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual void HandleStartingNewPlayer_Implementation(
        APlayerController* NewPlayer) override;

    virtual void Logout(AController* Exiting) override;

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

    static constexpr int32 RequiredPlayers = 4;
};
