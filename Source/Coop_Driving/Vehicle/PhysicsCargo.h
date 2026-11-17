#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicsCargo.generated.h"
class UStaticMeshComponent;
UCLASS()
class COOP_DRIVING_API APhysicsCargo : public AActor
{
    GENERATED_BODY()
public:
    APhysicsCargo();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cargo", meta=(ClampMin="1")) float MassKg = 25.f;
};
