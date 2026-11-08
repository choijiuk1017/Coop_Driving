#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Engine/EngineBaseTypes.h"
#include "Containers/Ticker.h"
#include "LANSessionSubsystem.generated.h"

// Local service: this instance survives map travel, but is not replicated.
UCLASS()
class COOP_DRIVING_API ULANSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void ConnectOrHost();
    void LeaveSession();
    void CloseLobby();
    void ReopenLobby();
    bool IsBusy() const { return bBusy; }
    bool IsHostingRoom() const { return bHosting; }
    bool IsAcceptingPlayers() const { return bHosting && bAccepting; }
    const FText& GetStatus() const { return Status; }
    static constexpr int32 PlayerLimit = 4;

private:
    bool EnsureInterface();
    void FindRooms();
    void OnFindComplete(bool bSuccess);
    void CreateRoom();
    void OnCreateComplete(FName Name, bool bSuccess);
    void OnJoinComplete(FName Name, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroyComplete(FName Name, bool bSuccess);
    void OnUpdateComplete(FName Name, bool bSuccess);
    void SetStatus(const FText& Message, bool bInBusy);
    void Fail(const FText& Message);
    void ReturnToMenu();
    void DestroyAndReturn();
    void NetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
    void TravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
    bool Tick(float DeltaTime);
    void RefreshAdvertisement();

    IOnlineSessionPtr Sessions;
    TSharedPtr<FOnlineSessionSearch> Search;
    FDelegateHandle FindHandle, CreateHandle, JoinHandle, DestroyHandle, UpdateHandle;
    FDelegateHandle NetworkHandle, TravelHandle;
    FTSTicker::FDelegateHandle TickHandle;
    FText Status;
    bool bBusy = false;
    bool bHosting = false;
    bool bAccepting = false;
    bool bConfirmSearch = false;
    bool bSearchScheduled = false;
    bool bUpdating = false;
    bool bReturning = false;
    bool bLeavingByUser = false;
    double NextSearchTime = 0;
    double NextAdvertiseTime = 0;
};
