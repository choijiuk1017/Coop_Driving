#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CoopDrivingGameInstance.generated.h"

UCLASS()
class COOP_DRIVING_API UCoopDrivingGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual TSubclassOf<AGameModeBase> OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,
        const FString& MapName, const FString& Options, const FString& Portal) const override;
};
