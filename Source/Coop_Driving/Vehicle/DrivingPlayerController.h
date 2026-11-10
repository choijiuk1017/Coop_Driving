#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Vehicle/DrivingInputRules.h"
#include "Player/PlayerRoleComponent.h"
#include "DrivingPlayerController.generated.h"

class ACoopVehicle;

UCLASS()
class COOP_DRIVING_API ADrivingPlayerController : public APlayerController
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

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Brake", meta = (ClampMin = "0.1"))
    float BrakeTapWindow = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Brake", meta = (ClampMin = "0.01", ClampMax = "1"))
    float BrakeGainPerTap = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Brake", meta = (ClampMin = "0.01"))
    float BrakeDecayPerSecond = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Gear")
    TArray<EGearArrow> ShiftUpPattern;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Gear")
    TArray<EGearArrow> ShiftDownPattern;

    UPROPERTY(EditDefaultsOnly, Category = "Driving|Gear", meta = (ClampMin = "0.1"))
    float GearInputTimeout = 2.f;

private:
    UPROPERTY(Replicated)
    TObjectPtr<ACoopVehicle> SharedVehicle;

    UPROPERTY(Replicated)
    TArray<EGearArrow> GearProgress;

    UPROPERTY(Replicated)
    FString GearFeedback;

    UFUNCTION(Server, Unreliable)
    void ServerDrivingState(bool bThrottle, float Steering);

    UFUNCTION(Server, Reliable)
    void ServerBrakeTap();

    UFUNCTION(Server, Reliable)
    void ServerGearArrow(EGearArrow Arrow);

    FBrakeTapState BrakeState;
    FGearPatternState GearState;
    EDrivingRole AppliedRole = EDrivingRole::None;
    double LastDrivingPacket = -1.0;
    float InputSendElapsed = 0.f;
    bool bPreviousThrottle = false;
    float PreviousSteering = 0.f;
};
