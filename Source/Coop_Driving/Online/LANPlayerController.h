#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LANPlayerController.generated.h"

// Session and voice behavior must survive the walking -> driving controller swap.
UCLASS()
class COOP_DRIVING_API ALANPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual bool InputKey(const FInputKeyParams& Params) override;
    virtual void ClientEnableNetworkVoice_Implementation(bool bEnable) override;
    virtual void ToggleSpeaking(bool bInSpeaking) override;

    UFUNCTION(Exec)
    void LANLeave();

    UFUNCTION(Exec)
    void LANVoiceStatus();

protected:
    void LeaveLANSession();
};
