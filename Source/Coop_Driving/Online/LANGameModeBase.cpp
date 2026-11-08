#include "Online/LANGameModeBase.h"
#include "Online/LANSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameSession.h"
#include "Engine/World.h"

void ALANGameModeBase::PreLogin(const FString& Options, const FString& Address,
    const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty()) return;
    if (GetNumPlayers() >= ULANSessionSubsystem::PlayerLimit)
    {
        ErrorMessage = TEXT("Room is full (4 players).");
        return;
    }
    const ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>();
    if (LAN && LAN->IsHostingRoom() && !LAN->IsAcceptingPlayers())
    {
        ErrorMessage = TEXT("Game has already started.");
    }
}

void ALANGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    // Also guard simultaneous admissions that passed PreLogin at the same time.
    if (GetNumPlayers() > ULANSessionSubsystem::PlayerLimit && GameSession)
    {
        GameSession->KickPlayer(NewPlayer, FText::FromString(TEXT("Room is full (4 players).")));
    }
}
