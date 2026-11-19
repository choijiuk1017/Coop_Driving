#pragma once

#include "CoreMinimal.h"
#include "Gimmick/Gimmick.h"
#include "DeerGimmick.generated.h"

class UStaticMeshComponent;

UCLASS()
class COOP_DRIVING_API ADeerGimmick : public AGimmick
{
    GENERATED_BODY()

public:
    ADeerGimmick();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Deer")
    TObjectPtr<UStaticMeshComponent> Body;

    // Actor +X is the crossing direction. Place at the roadside, facing across the road.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Deer", meta = (ClampMin = "1", Units = "cm/s"))
    float CrossingSpeed = 900.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Deer", meta = (ClampMin = "1", Units = "cm"))
    float CrossingDistance = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gimmick|Deer", meta = (ClampMin = "1", Units = "kg"))
    float MassKg = 70.f;

protected:
    virtual void BeginPlay() override;
    virtual void OnActivationChanged() override;
    virtual void UpdateGimmick(float DeltaSeconds) override;
    virtual void OnVehicleHit(ACoopVehicle* Vehicle, const FHitResult& Hit) override;

private:
    FVector CrossingStart = FVector::ZeroVector;
    FVector CrossingDirection = FVector::ForwardVector;
    float ActiveElapsed = 0.f;
    bool bConsumed = false;
};
