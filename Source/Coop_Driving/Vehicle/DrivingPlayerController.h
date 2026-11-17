#pragma once

#include "CoreMinimal.h"
#include "Online/LANPlayerController.h"
#include "Vehicle/DrivingInputRules.h"
#include "Player/PlayerRoleComponent.h"
#include "DrivingPlayerController.generated.h"

class ACoopVehicle;

UCLASS()
class COOP_DRIVING_API ADrivingPlayerController : public ALANPlayerController
{
    GENERATED_BODY()

public:
    ADrivingPlayerController();
    virtual void Tick(float DeltaTime) override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "Driving")
    EDrivingRole GetDrivingRole() const;

    void SetSharedVehicle(ACoopVehicle* Vehicle);
    void ClearDrivingInput();
    ACoopVehicle* GetSharedVehicle() const { return SharedVehicle; }
    FString GetDrivingHelp() const;

protected:
    virtual void BeginPlay() override;
    virtual void OnRep_PlayerState() override;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Brake", meta = (ClampMin = "0.1"))
    float BrakeTapWindow = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Brake", meta = (ClampMin = "0.01", ClampMax = "1"))
    float BrakeGainPerTap = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Brake", meta = (ClampMin = "0.01"))
    float BrakeDecayPerSecond = 0.6f;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Driving|Gear")
    TArray<EGearArrow> ShiftUpPattern;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Driving|Gear")
    TArray<EGearArrow> ShiftDownPattern;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Driving|Gear")
    TArray<EGearArrow> ReversePattern;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Gear", meta = (ClampMin = "0.1"))
    float GearInputTimeout = 2.f;

private:
    UPROPERTY(ReplicatedUsing = OnRep_SharedVehicle)
    TObjectPtr<ACoopVehicle> SharedVehicle;

    UFUNCTION()
    void OnRep_SharedVehicle();

    void RefreshRoleBinding();
    void HandleRoleAssignmentChanged();
    TWeakObjectPtr<UPlayerRoleComponent> BoundRoleComponent;

    UPROPERTY(Replicated)
    TArray<EGearArrow> GearProgress;

    UPROPERTY(Replicated)
    FString GearFeedback;

    UFUNCTION(Server, Unreliable)
    void ServerDrivingState(bool bThrottle, float Steering);

    UFUNCTION(Server, Reliable)
    void ServerBrakeTap();

    UFUNCTION(Server, Reliable)
    void ServerGearArrow(EGearArrow Arrow, uint32 ExpectedRevision);

    void RandomizeGearPatterns();
    UPROPERTY(Replicated)
    uint32 PatternRevision = 0;

    FBrakeTapState BrakeState;
    FGearPatternState GearState;
    EDrivingRole AppliedRole = EDrivingRole::None;
    double LastDrivingPacket = -1.0;
    float InputSendElapsed = 0.f;
    bool bPreviousThrottle = false;
    float PreviousSteering = 0.f;
};
