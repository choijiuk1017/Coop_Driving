#pragma once

#include "CoreMinimal.h"
#include "Gimmick/Gimmick.h"
#include "PoliceGimmick.generated.h"

class UStaticMeshComponent;

/** A timed pursuit that automatically starts when the vehicle reaches its placed approach zone. */
UCLASS()
class COOP_DRIVING_API APoliceGimmick : public AGimmick
{
    GENERATED_BODY()

public:
    APoliceGimmick();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Police")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Police", meta = (ClampMin = "0.1", Units = "s"))
    float PursuitDuration = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Police", meta = (ClampMin = "1", Units = "cm/s"))
    float PursuitSpeed = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Police", meta = (ClampMin = "1", Units = "cm"))
    float StartDistanceBehind = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Police", meta = (ClampMin = "1", Units = "cm"))
    float CatchDistance = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Police")
    bool bStartBehindVehicle = true;

    // False leaves only the effect event, where a cargo/currency penalty can be supplied in BP.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Police")
    bool bGameOverOnCatch = true;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Police")
    bool bCaughtVehicle = false;

protected:
    virtual void OnActivationChanged() override;
    virtual void UpdateGimmick(float DeltaSeconds) override;
    virtual void OnVehicleHit(ACoopVehicle* Vehicle, const FHitResult& Hit) override;
    virtual void ApplyEffect(ACoopVehicle* Vehicle) override;

private:
    void CatchVehicle(ACoopVehicle* Vehicle);
    FVector GroundedPosition(const FVector& Position, ACoopVehicle* Vehicle) const;
    float PursuitElapsed = 0.f;
    bool bPursuitStarted = false;
};
