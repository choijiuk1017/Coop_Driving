#pragma once
#include "CoreMinimal.h"
#include "Gimmick/Gimmick.h"
#include "CargoSpeedBump.generated.h"
class UStaticMeshComponent;
UCLASS(HideCategories = ("Gimmick|Activation"))
class COOP_DRIVING_API ACargoSpeedBump : public AGimmick
{
    GENERATED_BODY()
public:
    ACargoSpeedBump();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Surface;
protected:
    virtual void OnActivationChanged() override;
};
