#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DrivingGameMode.generated.h"

class APlayerController;
class AController;

UCLASS()
class COOP_DRIVING_API ADrivingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void HandleStartingNewPlayer_Implementation(
        APlayerController* NewPlayer) override;

    virtual void Logout(AController* Exiting) override;

private:
    void TryAssignRoles();

    TArray<TWeakObjectPtr<APlayerController>> ArrivedPlayers;

    bool bRolesAssigned = false;

    static constexpr int32 RequiredPlayers = 4;
};