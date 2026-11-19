#pragma once

#include "CoreMinimal.h"
#include "Gimmick/Gimmick.h"
#include "TrafficLightGimmick.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETrafficSignal : uint8
{
    Red,
    Green,
    Yellow
};

/** Local X=0 is the stop line. Vehicles approach along local +X, from X<0. */
UCLASS()
class COOP_DRIVING_API ATrafficLightGimmick : public AGimmick
{
    GENERATED_BODY()

public:
    ATrafficLightGimmick();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Gimmick|Traffic")
    void SetSignal(ETrafficSignal NewSignal);

    UFUNCTION(BlueprintPure, Category = "Gimmick|Traffic")
    ETrafficSignal GetSignal() const { return Signal; }

    // Segment intersection also catches a vehicle skipping the whole zone in a fast frame.
    bool CrossesStopLine(const FVector& PreviousWorldLocation, const FVector& WorldLocation) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Traffic")
    TObjectPtr<UBoxComponent> StopZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Traffic")
    TObjectPtr<UStaticMeshComponent> SignalPost;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Traffic")
    TObjectPtr<UPointLightComponent> RedLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Traffic")
    TObjectPtr<UPointLightComponent> GreenLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Traffic")
    TObjectPtr<UPointLightComponent> YellowLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic")
    bool bAutomaticCycle = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic", meta = (ClampMin = "0.1", Units = "s"))
    float RedDuration = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic", meta = (ClampMin = "0.1", Units = "s"))
    float GreenDuration = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic", meta = (ClampMin = "0.1", Units = "s"))
    float YellowDuration = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic")
    bool bTreatYellowAsRed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic", meta = (ClampMin = "0", Units = "cm/s"))
    float StoppedSpeedTolerance = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Traffic", meta = (ClampMin = "0", Units = "s"))
    float RequiredStopTime = 0.5f;

protected:
    virtual void BeginPlay() override;
    virtual void UpdateGimmick(float DeltaSeconds) override;
    virtual void OnActivationChanged() override;
    virtual void ApplyEffect(ACoopVehicle* Vehicle) override;

    UFUNCTION(BlueprintImplementableEvent, Category = "Gimmick|Traffic")
    void OnVehicleStopped(ACoopVehicle* Vehicle);

    UPROPERTY(ReplicatedUsing = RefreshSignal, EditAnywhere, BlueprintReadOnly, Category = "Gimmick|Traffic")
    ETrafficSignal Signal = ETrafficSignal::Red;

private:
    UFUNCTION()
    void RefreshSignal();

    TWeakObjectPtr<ACoopVehicle> ObservedVehicle;
    FVector PreviousLocation = FVector::ZeroVector;
    float PhaseElapsed = 0.f;
    float StoppedElapsed = 0.f;
    bool bHasPreviousLocation = false;
    bool bReportedStop = false;
};
