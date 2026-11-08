#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LANGameModeBase.generated.h"

// Shared admission policy for the lobby and gameplay map.
UCLASS()
class COOP_DRIVING_API ALANGameModeBase : public AGameModeBase
{
    GENERATED_BODY()
public:
    virtual void PreLogin(const FString& Options, const FString& Address,
        const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
};
