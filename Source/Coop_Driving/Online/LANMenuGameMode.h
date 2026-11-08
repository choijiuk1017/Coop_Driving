#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "LANMenuGameMode.generated.h"

class SWidget;
class SButton;

UCLASS()
class COOP_DRIVING_API ALANMenuPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UFUNCTION(Exec)
    void LANConnect();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TSharedPtr<SWidget> MenuWidget;
    TSharedPtr<SButton> ConnectButton;
};

UCLASS()
class COOP_DRIVING_API ALANMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ALANMenuGameMode();
};
