#include "Online/GameplayVoiceSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/LobbyGameMode.h"
#include "Player/MainPlayerController.h"
#include "Player/PlayerRoleComponent.h"

void UGameplayVoiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &ThisClass::Tick), 0.2f);
}

void UGameplayVoiceSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    StopRoomVoice();
    Super::Deinitialize();
}

bool UGameplayVoiceSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer) return true;
    APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
    if (!PC || !PC->IsLocalController() || !PC->IsA<AMainPlayerController>()) return true;
    APlayerState* PS = PC->PlayerState;
    if (!IsValid(PS) || !PS->GetUniqueId().IsValid()) return true;

    if (!EnsureVoiceSession())
    {
        if (!bWarnedNoSession)
        {
            UE_LOG(LogTemp, Log, TEXT("[Voice] Waiting for a voice session. Normal game: connect through MenuMap; editor: use Play As Listen Server."));
            bWarnedNoSession = true;
        }
        return true;
    }

    const AGameStateBase* GS = World->GetGameState();
    const bool bLobby = GS && GS->GameModeClass && GS->GameModeClass->IsChildOf(ALobbyGameMode::StaticClass());
    UPlayerRoleComponent* Roles = PS->FindComponentByClass<UPlayerRoleComponent>();
    const bool bAssigned = Roles && Roles->GetAssignment().Role != EDrivingRole::None;
    if (!bRoomVoiceStarted)
    {
        if (!bLobby && !bAssigned) return true;
        bRoomVoiceStarted = true;
        UE_LOG(LogTemp, Log, TEXT("[Voice] Room voice enabled for local player %s (open mic)."), *PS->GetPlayerName());
    }

    // Also covers RepNotify arriving before controller/UniqueId initialization.
    if (bAssigned) Roles->RefreshLocalVoiceEffects();

    if (!Voice.IsValid()) Voice = Online::GetVoiceInterface(World, FName(TEXT("NULL")));
    if (!Voice.IsValid())
    {
        if (!bWarnedUnavailable)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Voice] Voice interface unavailable. Check the default microphone, audio device and OS microphone access; restart after fixing the device."));
            bWarnedUnavailable = true;
        }
        return true;
    }
    if (!bRegistered || RegisteredPlayerState.Get() != PS)
    {
        bRegistered = Voice->RegisterLocalTalker(0);
        if (!bRegistered) return true;
        RegisteredPlayerState = PS;
        RegisteredRemoteTalkers.Empty();
        // Null registration automatically starts capture; immediately restore policy.
        ApplyTransmitPolicy(true);
        UE_LOG(LogTemp, Log, TEXT("[Voice] Local talker registered. %s"), *GetDebugState());
    }
    ApplyTransmitPolicy(false);
    ApplyReceivePolicy(PC);
    return true;
}

void UGameplayVoiceSubsystem::RefreshPolicy(bool bForceTransmit)
{
    if (!bRoomVoiceStarted || !Voice.IsValid() || !bRegistered) return;
    ApplyTransmitPolicy(bForceTransmit);
    APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
    if (PC && PC->IsLocalController()) ApplyReceivePolicy(PC);
}

void UGameplayVoiceSubsystem::ApplyTransmitPolicy(bool bForce)
{
    const bool bAllowed = bRoomVoiceStarted && !bMute;
    if (!Voice.IsValid() || (!bForce && bTransmitKnown && bAllowed == bLastTransmitAllowed)) return;
    if (bAllowed) Voice->StartNetworkedVoice(0);
    else Voice->StopNetworkedVoice(0);
    bLastTransmitAllowed = bAllowed;
    bTransmitKnown = true;
    UE_LOG(LogTemp, Log, TEXT("[Voice] Local transmit=%s receive=%s"),
        bAllowed ? TEXT("ON") : TEXT("OFF"), bDeaf ? TEXT("OFF") : TEXT("ON"));
}

void UGameplayVoiceSubsystem::ApplyReceivePolicy(APlayerController* PC)
{
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!GS || !Voice.IsValid()) return;
    // Direct PIE connections predate our hidden session, so engine session login
    // may not have registered these talkers. Also handles late player IDs/travel.
    for (APlayerState* Remote : GS->PlayerArray)
    {
        if (!IsValid(Remote) || Remote == PC->PlayerState || !Remote->GetUniqueId().IsValid()) continue;
        const FString Key = Remote->GetUniqueId().GetUniqueNetId()->ToString();
        if (RegisteredRemoteTalkers.FindRef(Key).Get() != Remote
            && Voice->RegisterRemoteTalker(*Remote->GetUniqueId().GetUniqueNetId()))
            RegisteredRemoteTalkers.Add(Key, Remote);
    }
    if (!bDeaf)
    {
        for (const auto& Entry : MutedByDisability)
        {
            if (Entry.Value.IsValid() && !PC->IsPlayerMuted(*Entry.Value.GetUniqueNetId()))
                Voice->UnmuteRemoteTalker(0, *Entry.Value.GetUniqueNetId(), false);
        }
        MutedByDisability.Empty();
        return;
    }
    for (APlayerState* Remote : GS->PlayerArray)
    {
        if (!IsValid(Remote) || Remote == PC->PlayerState || !Remote->GetUniqueId().IsValid()) continue;
        const FUniqueNetId& Id = *Remote->GetUniqueId().GetUniqueNetId();
        if (!Voice->IsMuted(0, Id) && Voice->MuteRemoteTalker(0, Id, false))
        {
            MutedByDisability.Add(Id.ToString(), Remote->GetUniqueId());
            UE_LOG(LogTemp, Log, TEXT("[Voice] Deaf: muted remote %s locally."), *Remote->GetPlayerName());
        }
    }
}

void UGameplayVoiceSubsystem::StopRoomVoice()
{
    if (Voice.IsValid())
    {
        // Called before session destruction, while Null still allows unmuting.
        APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
        for (const auto& Entry : MutedByDisability)
        {
            if (Entry.Value.IsValid() && (!PC || !PC->IsPlayerMuted(*Entry.Value.GetUniqueNetId())))
                Voice->UnmuteRemoteTalker(0, *Entry.Value.GetUniqueNetId(), false);
        }
        Voice->StopNetworkedVoice(0);
    }
    MutedByDisability.Empty();
    RegisteredRemoteTalkers.Empty();
    RegisteredPlayerState.Reset();
    bRoomVoiceStarted = bRegistered = bTransmitKnown = bDeaf = bMute = false;
    bWarnedNoSession = false;
    ReleasePIEVoiceSession();
    // PIE destroys its per-world Null subsystem immediately after GameInstance
    // shutdown. Release our shared reference before that subsystem checks ownership.
    Voice.Reset();
}

FString UGameplayVoiceSubsystem::GetDebugState() const
{
    return FString::Printf(TEXT("Room=%d Registered=%d Mute=%d Deaf=%d Interface=%d\n%s"),
        bRoomVoiceStarted, bRegistered, bMute, bDeaf, Voice.IsValid(),
        Voice.IsValid() ? *Voice->GetVoiceDebugState() : TEXT("No voice interface"));
}
