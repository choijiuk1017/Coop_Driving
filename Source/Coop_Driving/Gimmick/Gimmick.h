#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gimmick.generated.h"

class ACoopVehicle;
class UPrimitiveComponent;
class UBoxComponent;
class UArrowComponent;

UENUM(BlueprintType)
enum class EGimmickActivationMode : uint8
{
    Immediate,
    VehicleApproach
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGimmickEffectApplied, ACoopVehicle*);

/** Shared lifecycle and server-authoritative behaviour for road hazards. */
UCLASS(Abstract, Blueprintable)
class COOP_DRIVING_API AGimmick : public AActor
{
    GENERATED_BODY()

public:
    AGimmick();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Enable/disable on the server. Approach mode waits in its built-in zone when enabled.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Gimmick")
    void SetGimmickActive(bool bNewActive);

    UFUNCTION(BlueprintPure, Category = "Gimmick")
    bool IsGimmickActive() const { return bActive; }

    UFUNCTION(BlueprintPure, Category = "Gimmick")
    bool IsWaitingForVehicle() const { return bWaitingForVehicle; }

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Gimmick")
    void SetTargetVehicle(ACoopVehicle* Vehicle);

    UFUNCTION(BlueprintPure, Category = "Gimmick")
    ACoopVehicle* GetTargetVehicle() const;

    // Native gameplay subscribers are notified only on authority.
    FOnGimmickEffectApplied OnEffectApplied;

    // Visible in the level editor; no external trigger actor or overlap configuration is required.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Activation")
    TObjectPtr<UBoxComponent> ActivationZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gimmick|Activation")
    TObjectPtr<UArrowComponent> DirectionArrow;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gimmick|Activation")
    EGimmickActivationMode ActivationMode = EGimmickActivationMode::Immediate;

    bool IsInActivationZone(const FVector& WorldLocation) const;

protected:
    virtual void BeginPlay() override;
    virtual void UpdateGimmick(float DeltaSeconds) {}
    virtual void OnActivationChanged() {}
    virtual void OnVehicleHit(ACoopVehicle* Vehicle, const FHitResult& Hit) {}
    virtual void ApplyEffect(ACoopVehicle* Vehicle);

    bool IsTargetVehicle(const ACoopVehicle* Vehicle) const;
    void BindVehicleCollision(UPrimitiveComponent* Component);
    void SetGimmickRoot(USceneComponent* NewRoot);

    // Cosmetic notification on server AND clients. Gameplay belongs in ApplyEffect on the server.
    UFUNCTION(BlueprintImplementableEvent, Category = "Gimmick", meta = (DisplayName = "On Gimmick Triggered"))
    void ReceiveGimmickTriggered(ACoopVehicle* Vehicle);

    // Server-only BP extension for cargo/currency penalties; cosmetics use the event above.
    UFUNCTION(BlueprintImplementableEvent, Category = "Gimmick", meta = (DisplayName = "Apply Gimmick Effect"))
    void ReceiveApplyEffect(ACoopVehicle* Vehicle);

    UPROPERTY(ReplicatedUsing = OnRep_Active, EditAnywhere, BlueprintReadOnly, Category = "Gimmick")
    bool bActive = true;

    // Empty automatically uses DrivingGameMode's shared vehicle, or a placed CoopVehicle in a test map.
    UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadOnly, Category = "Gimmick")
    TObjectPtr<ACoopVehicle> TargetVehicle;

private:
    UPROPERTY(ReplicatedUsing = OnRep_Active)
    bool bWaitingForVehicle = false;

    bool CheckVehicleApproach();
    void ResetApproachObservation();
    TWeakObjectPtr<ACoopVehicle> ApproachVehicle;
    FVector PreviousApproachLocation = FVector::ZeroVector;
    bool bHasApproachLocation = false;
    mutable TWeakObjectPtr<ACoopVehicle> AutoDetectedVehicle;
    mutable double NextVehicleSearchTime = -1.0;

    UFUNCTION()
    void OnRep_Active();

    UFUNCTION()
    void HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastTriggered(ACoopVehicle* Vehicle);
};
