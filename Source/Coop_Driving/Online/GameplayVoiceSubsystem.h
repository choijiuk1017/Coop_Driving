#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/VoiceInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemTypes.h"
#include "Containers/Ticker.h"
#include "GameplayVoiceSubsystem.generated.h"

class APlayerController;
class APlayerState;

// One local user per game instance. Open-mic lobby, then local role restrictions.
// Restrictions deliberately survive map travel: returning to the lobby does NOT reset them.
UCLASS(Config=Game)
class COOP_DRIVING_API UGameplayVoiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void SetDeaf(bool bEnabled) { bDeaf = bEnabled; }
    void SetMute(bool bEnabled) { bMute = bEnabled; }
    void RefreshPolicy(bool bForceTransmit = false);
    void StopRoomVoice();
    FString GetDebugState() const;

private:
    // Editor-only convenience; packaged games always require the real LAN session.
    UPROPERTY(Config)
    bool bEnablePIEVoiceSession = true;

    bool EnsureVoiceSession();
    void ReleasePIEVoiceSession();
#if WITH_EDITOR
    void OnPIEVoiceSessionCreated(FName SessionName, bool bSuccessful);
    IOnlineSessionPtr PIEVoiceSessions;
    FDelegateHandle PIECreateHandle;
    bool bOwnsPIEVoiceSession = false;
    bool bPIECreatePending = false;
    bool bPIECreateFailed = false;
#endif
    bool Tick(float DeltaTime);
    void ApplyTransmitPolicy(bool bForce);
    void ApplyReceivePolicy(APlayerController* LocalController);
    IOnlineVoicePtr Voice;
    FTSTicker::FDelegateHandle TickHandle;
    TWeakObjectPtr<APlayerState> RegisteredPlayerState;
    TMap<FString, FUniqueNetIdRepl> MutedByDisability;
    TMap<FString, TWeakObjectPtr<APlayerState>> RegisteredRemoteTalkers;
    bool bRoomVoiceStarted = false;
    bool bDeaf = false;
    bool bMute = false;
    bool bRegistered = false;
    bool bTransmitKnown = false;
    bool bLastTransmitAllowed = false;
    bool bWarnedUnavailable = false;
    bool bWarnedNoSession = false;
};
