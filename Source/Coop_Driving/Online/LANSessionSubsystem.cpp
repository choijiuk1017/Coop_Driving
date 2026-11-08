#include "Online/LANSessionSubsystem.h"
#include "Online/GameplayVoiceSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "CoopLAN"
namespace CoopLAN
{
    const FName ProductKey(TEXT("COOP_PRODUCT"));
    const FString Product(TEXT("CoopDrivingLAN_v1"));
    const FName CountKey(TEXT("COOP_PLAYERS"));
    const FName OpenKey(TEXT("COOP_OPEN"));
    const FName MenuMap(TEXT("/Game/Maps/MenuMap"));
    const FName LobbyMap(TEXT("/Game/Maps/Untitled"));
}

void ULANSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Status = LOCTEXT("Ready", "같은 로컬 네트워크의 방에 접속합니다.");
    if (GEngine)
    {
        NetworkHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::NetworkFailure);
        TravelHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::TravelFailure);
    }
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick));
}

void ULANSessionSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().Remove(NetworkHandle);
        GEngine->OnTravelFailure().Remove(TravelHandle);
    }
    if (Sessions.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
        Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
    }
    Super::Deinitialize();
}

bool ULANSessionSubsystem::EnsureInterface()
{
    if (!Sessions.IsValid())
    {
        Sessions = Online::GetSessionInterface(GetWorld(), FName(TEXT("NULL")));
    }
    if (!Sessions.IsValid())
    {
        SetStatus(LOCTEXT("NoService", "LAN 세션 서비스를 사용할 수 없습니다."), false);
        return false;
    }
    return true;
}

void ULANSessionSubsystem::SetStatus(const FText& Message, bool bInBusy)
{
    Status = Message;
    bBusy = bInBusy;
    UE_LOG(LogTemp, Log, TEXT("[LAN] %s"), *Message.ToString());
}

void ULANSessionSubsystem::ConnectOrHost()
{
    if (bBusy || !EnsureInterface()) return;
    if (Sessions->GetNamedSession(NAME_GameSession))
    {
        Fail(LOCTEXT("Existing", "기존 세션을 정리했습니다. 다시 접속해 주세요."));
        return;
    }
    bConfirmSearch = false;
    bReturning = false;
    FindRooms();
}

void ULANSessionSubsystem::FindRooms()
{
    Search = MakeShared<FOnlineSessionSearch>();
    Search->bIsLanQuery = true;
    Search->MaxSearchResults = 100;
    Search->TimeoutInSeconds = 3.0f;
    SetStatus(LOCTEXT("Searching", "로컬 네트워크의 방을 찾는 중..."), true);
    FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindComplete));
    if (!Sessions->FindSessions(0, Search.ToSharedRef()))
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
        SetStatus(LOCTEXT("SearchFailed", "검색을 시작하지 못했습니다. 다시 시도해 주세요."), false);
    }
}

void ULANSessionSubsystem::OnFindComplete(bool bSuccess)
{
    Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
    if (!bSuccess)
    {
        SetStatus(LOCTEXT("SearchError", "LAN 검색에 실패했습니다. 다시 시도해 주세요."), false);
        return; // A failed search must never create another room.
    }

    TArray<FOnlineSessionSearchResult> Rooms;
    for (const auto& Result : Search->SearchResults)
    {
        FString Product;
        if (Result.IsValid() && Result.Session.SessionSettings.Get(CoopLAN::ProductKey, Product)
            && Product == CoopLAN::Product)
        {
            Rooms.Add(Result);
        }
    }
    // If an earlier race produced several rooms, all clients choose the same ID.
    Rooms.Sort([](const FOnlineSessionSearchResult& A, const FOnlineSessionSearchResult& B)
    {
        return A.GetSessionIdStr() < B.GetSessionIdStr();
    });
    if (!Rooms.IsEmpty())
    {
        const auto& Room = Rooms[0];
        int32 Count = PlayerLimit;
        bool bOpen = false;
        Room.Session.SessionSettings.Get(CoopLAN::CountKey, Count);
        Room.Session.SessionSettings.Get(CoopLAN::OpenKey, bOpen);
        if (Count >= PlayerLimit || !bOpen)
        {
            SetStatus(LOCTEXT("Unavailable", "기존 방이 가득 찼거나 게임 중입니다. 새 방은 생성하지 않습니다."), false);
            return;
        }
        SetStatus(LOCTEXT("Joining", "기존 방에 접속하는 중..."), true);
        JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
            FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinComplete));
        if (!Sessions->JoinSession(0, NAME_GameSession, Room))
        {
            Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
            Fail(LOCTEXT("JoinStartError", "방 접속을 시작하지 못했습니다."));
        }
        return;
    }
    if (!bConfirmSearch)
    {
        bConfirmSearch = true;
        bSearchScheduled = true;
        NextSearchTime = FPlatformTime::Seconds() + FMath::FRandRange(0.4f, 1.4f);
        SetStatus(LOCTEXT("Confirm", "방이 없는지 한 번 더 확인하는 중..."), true);
        return;
    }
    CreateRoom();
}

void ULANSessionSubsystem::CreateRoom()
{
    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = true;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;
    Settings.bUsesPresence = false;
    Settings.bUseLobbiesIfAvailable = false;
    // Null only returns rooms with a free public slot. Reserve one discovery slot;
    // actual gameplay admission is capped at four by ALANGameModeBase::PreLogin.
    Settings.NumPublicConnections = PlayerLimit + 1;
    Settings.Set(CoopLAN::ProductKey, CoopLAN::Product, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(CoopLAN::CountKey, 1, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(CoopLAN::OpenKey, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    SetStatus(LOCTEXT("Creating", "방이 없어 리슨 서버를 만드는 중..."), true);
    CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateComplete));
    if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
    {
        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        Fail(LOCTEXT("CreateStartError", "방 생성을 시작하지 못했습니다."));
    }
}

void ULANSessionSubsystem::OnCreateComplete(FName Name, bool bSuccess)
{
    Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
    if (!bSuccess) { Fail(LOCTEXT("CreateError", "방 생성에 실패했습니다.")); return; }
    bHosting = true;
    bAccepting = true;
    NextAdvertiseTime = FPlatformTime::Seconds() + 1.0;
    SetStatus(LOCTEXT("Hosting", "호스트로 로비를 여는 중..."), true);
    UGameplayStatics::OpenLevel(GetGameInstance(), CoopLAN::LobbyMap, true, TEXT("listen"));
}

void ULANSessionSubsystem::OnJoinComplete(FName Name, EOnJoinSessionCompleteResult::Type Result)
{
    Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
    FString Address;
    APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController();
    if (Result != EOnJoinSessionCompleteResult::Success || !Controller
        || !Sessions->GetResolvedConnectString(NAME_GameSession, Address) || Address.IsEmpty())
    {
        Fail(LOCTEXT("JoinError", "방에 접속하지 못했습니다. 잠시 후 다시 시도해 주세요."));
        return;
    }
    SetStatus(LOCTEXT("Travel", "호스트의 로비로 이동하는 중..."), true);
    Controller->ClientTravel(Address, TRAVEL_Absolute);
}

void ULANSessionSubsystem::CloseLobby()
{
    bAccepting = false;
    RefreshAdvertisement();
}

void ULANSessionSubsystem::ReopenLobby()
{
    bAccepting = bHosting;
    RefreshAdvertisement();
}

bool ULANSessionSubsystem::Tick(float DeltaTime)
{
    const double Now = FPlatformTime::Seconds();
    if (bSearchScheduled && Now >= NextSearchTime)
    {
        bSearchScheduled = false;
        FindRooms();
    }
    if (bHosting && Now >= NextAdvertiseTime)
    {
        NextAdvertiseTime = Now + 1.0;
        RefreshAdvertisement();
    }
    return true;
}

void ULANSessionSubsystem::RefreshAdvertisement()
{
    if (!bHosting || !Sessions.IsValid() || bUpdating || !GetWorld()) return;
    FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
    if (!Session) return;
    int32 Count = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        if (PC && PC->PlayerState && !PC->PlayerState->IsOnlyASpectator()) ++Count;
    }
    int32 PreviousCount = -1;
    bool bPreviousOpen = !bAccepting;
    Session->SessionSettings.Get(CoopLAN::CountKey, PreviousCount);
    Session->SessionSettings.Get(CoopLAN::OpenKey, bPreviousOpen);
    if (PreviousCount == Count && bPreviousOpen == bAccepting) return;
    FOnlineSessionSettings Settings = Session->SessionSettings;
    Settings.Set(CoopLAN::CountKey, Count, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    Settings.Set(CoopLAN::OpenKey, bAccepting, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    bUpdating = true;
    UpdateHandle = Sessions->AddOnUpdateSessionCompleteDelegate_Handle(
        FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnUpdateComplete));
    if (!Sessions->UpdateSession(NAME_GameSession, Settings, true))
    {
        Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
        bUpdating = false;
    }
}

void ULANSessionSubsystem::OnUpdateComplete(FName Name, bool bSuccess)
{
    Sessions->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateHandle);
    bUpdating = false;
}

void ULANSessionSubsystem::Fail(const FText& Message)
{
    SetStatus(Message, false);
    if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession)) DestroyAndReturn();
}

void ULANSessionSubsystem::LeaveSession()
{
    bLeavingByUser = true;
    SetStatus(LOCTEXT("Leaving", "방에서 나가는 중..."), true);
    DestroyAndReturn();
}

void ULANSessionSubsystem::DestroyAndReturn()
{
    if (bReturning) return;
    GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->StopRoomVoice();
    bReturning = true;
    bHosting = false;
    bAccepting = false;
    bSearchScheduled = false;
    if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
    {
        DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyComplete));
        if (Sessions->DestroySession(NAME_GameSession)) return;
        Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
    }
    ReturnToMenu();
}

void ULANSessionSubsystem::OnDestroyComplete(FName Name, bool bSuccess)
{
    Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
    ReturnToMenu();
}

void ULANSessionSubsystem::ReturnToMenu()
{
    bBusy = false;
    bReturning = false;
    if (bLeavingByUser)
    {
        bLeavingByUser = false;
        SetStatus(LOCTEXT("Ready", "같은 로컬 네트워크의 방에 접속합니다."), false);
    }
    Search.Reset();
    UGameplayStatics::OpenLevel(GetGameInstance(), CoopLAN::MenuMap, true);
}

void ULANSessionSubsystem::NetworkFailure(UWorld* World, UNetDriver* Driver,
    ENetworkFailure::Type Type, const FString& Error)
{
    if (!World || World->GetGameInstance() != GetGameInstance() || bReturning) return;
    SetStatus(LOCTEXT("Disconnected", "서버 연결이 끊어졌습니다. 다시 접속할 수 있습니다."), true);
    DestroyAndReturn();
}

void ULANSessionSubsystem::TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
    if (!World || World->GetGameInstance() != GetGameInstance() || bReturning) return;
    SetStatus(LOCTEXT("TravelFailed", "맵 이동에 실패했습니다. 다시 시도해 주세요."), true);
    DestroyAndReturn();
}
#undef LOCTEXT_NAMESPACE
