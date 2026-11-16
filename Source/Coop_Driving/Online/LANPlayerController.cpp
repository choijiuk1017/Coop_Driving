#include "Online/LANPlayerController.h"
#include "Online/LANSessionSubsystem.h"
#include "Online/GameplayVoiceSubsystem.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerInput.h"

void ALANPlayerController::LeaveLANSession()
{
    if (IsLocalController())
    {
        GetGameInstance()->GetSubsystem<ULANSessionSubsystem>()->LeaveSession();
    }
}

bool ALANPlayerController::InputKey(const FInputKeyParams& Params)
{
    if (Params.Key == EKeys::Escape && Params.Event == IE_Pressed)
    {
        LeaveLANSession();
        return true;
    }
    return Super::InputKey(Params);
}

void ALANPlayerController::LANLeave()
{
    LeaveLANSession();
}

void ALANPlayerController::ClientEnableNetworkVoice_Implementation(bool bEnable)
{
    // Keep engine handshakes from overriding the local open-mic/disability policy.
    ToggleSpeaking(bEnable);
}

void ALANPlayerController::ToggleSpeaking(bool bInSpeaking)
{
    // Open mic has no push-to-talk input: only the role policy controls transmission.
    GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->RefreshPolicy(true);
}

void ALANPlayerController::LANVoiceStatus()
{
    UE_LOG(LogTemp, Log, TEXT("[Voice] %s"),
        *GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->GetDebugState());
}
