#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DrivingDestinationZone.generated.h"

class UBoxComponent;

// Place one in each hand-authored driving map and resize DestinationBounds.
// The shared vehicle's origin must enter this box. No vehicle collision edits required.
UCLASS()
class COOP_DRIVING_API ADrivingDestinationZone : public AActor
{
    GENERATED_BODY()

public:
    ADrivingDestinationZone();
    bool ContainsLocation(const FVector& WorldLocation) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destination")
    TObjectPtr<UBoxComponent> DestinationBounds;

private:
    void CheckDestination();
    FTimerHandle CheckTimer;
};
