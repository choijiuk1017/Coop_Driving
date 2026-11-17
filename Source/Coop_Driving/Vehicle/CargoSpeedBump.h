#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CargoSpeedBump.generated.h"
class UStaticMeshComponent;
UCLASS()
class COOP_DRIVING_API ACargoSpeedBump : public AActor
{
    GENERATED_BODY()
public:
    ACargoSpeedBump();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Surface;
};
