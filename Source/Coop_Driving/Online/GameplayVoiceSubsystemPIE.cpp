#include "Online/GameplayVoiceSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    const FName PIEVoiceMarker(TEXT("COOP_PIE_VOICE"));
}

bool UGameplayVoiceSubsystem::EnsureVoiceSession()
{
    UWorld* World = GetWorld();
    IOnlineSessionPtr Sessions = World ? Online::GetSessionInterface(World, FName(TEXT("NULL"))) : nullptr;
    if (!Sessions.IsValid()) return false;
    // A real LAN session always takes priority and is never altered here.
    if (Sessions->GetNamedSession(NAME_GameSession))
    {
#if WITH_EDITOR
        if (bPIECreatePending) return false;
#endif
        return true;
    }

#if WITH_EDITOR
    const bool bPIELaunch = World->WorldType == EWorldType::PIE
        || FParse::Param(FCommandLine::Get(), TEXT("PIEVIACONSOLE"));
    const ENetMode NetMode = World->GetNetMode();
    if (!bEnablePIEVoiceSession || !bPIELaunch
        || (NetMode != NM_ListenServer && NetMode != NM_Client)
        || bPIECreatePending || bPIECreateFailed) return false;

    PIEVoiceSessions = Sessions;
    bPIECreatePending = true;
    PIECreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnPIEVoiceSessionCreated));
    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = false;
    Settings.bShouldAdvertise = false;
    Settings.bUsesPresence = false;
    Settings.bAllowInvites = false;
    Settings.NumPublicConnections = 4;
    Settings.Set(PIEVoiceMarker, true, EOnlineDataAdvertisementType::DontAdvertise);
    // Null can invoke the completion delegate synchronously.
    if (!Sessions->CreateSession(0, NAME_GameSession, Settings) && bPIECreatePending)
        OnPIEVoiceSessionCreated(NAME_GameSession, false);
    return bOwnsPIEVoiceSession;
#else
    return false;
#endif
}

#if WITH_EDITOR
void UGameplayVoiceSubsystem::OnPIEVoiceSessionCreated(FName SessionName, bool bSuccessful)
{
    if (SessionName != NAME_GameSession || !bPIECreatePending) return;
    PIEVoiceSessions->ClearOnCreateSessionCompleteDelegate_Handle(PIECreateHandle);
    PIECreateHandle.Reset();
    bPIECreatePending = false;
    FNamedOnlineSession* Session = PIEVoiceSessions->GetNamedSession(SessionName);
    bool bMarked = false;
    bOwnsPIEVoiceSession = bSuccessful && Session
        && Session->SessionSettings.Get(PIEVoiceMarker, bMarked) && bMarked;
    bPIECreateFailed = !bOwnsPIEVoiceSession;
    UE_LOG(LogTemp, Log, TEXT("[Voice][PIE] Hidden test session %s. World=%s (no LAN advertisement or travel)"),
        bOwnsPIEVoiceSession ? TEXT("ready") : TEXT("failed"), *GetWorld()->GetName());
}
#endif

void UGameplayVoiceSubsystem::ReleasePIEVoiceSession()
{
#if WITH_EDITOR
    if (PIEVoiceSessions.IsValid())
    {
        PIEVoiceSessions->ClearOnCreateSessionCompleteDelegate_Handle(PIECreateHandle);
        FNamedOnlineSession* Session = PIEVoiceSessions->GetNamedSession(NAME_GameSession);
        bool bMarked = false;
        if ((bOwnsPIEVoiceSession || bPIECreatePending) && Session
            && Session->SessionSettings.Get(PIEVoiceMarker, bMarked) && bMarked)
        {
            PIEVoiceSessions->DestroySession(NAME_GameSession);
            UE_LOG(LogTemp, Log, TEXT("[Voice][PIE] Hidden test session released."));
        }
    }
    PIECreateHandle.Reset();
    PIEVoiceSessions.Reset();
    bOwnsPIEVoiceSession = bPIECreatePending = bPIECreateFailed = false;
#endif
}
