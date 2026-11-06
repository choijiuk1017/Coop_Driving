// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DrivingPlayerController.generated.h"


class UInputMappingContext;
class UInputAction;
class ACoopVehicle;
/**
 * 
 */
UCLASS()
class COOP_DRIVING_API ADrivingPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    ADrivingPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DrivingMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ThrottleAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> BrakeAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> SteerLeftAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> SteerRightAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> GearUpAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> GearDownAction;

    UPROPERTY(EditDefaultsOnly, Category = "Vehicle")
    int32 MaxForwardGear = 8;

private:
    void PressThrottle();
    void ReleaseThrottle();

    void PressBrake();
    void ReleaseBrake();

    void PressSteerLeft();
    void ReleaseSteerLeft();

    void PressSteerRight();
    void ReleaseSteerRight();

    void UpdateSteering();

    void GearUp();
    void GearDown();

    UFUNCTION(Server, Reliable)
    void ServerSetThrottle(float Value);

    UFUNCTION(Server, Reliable)
    void ServerSetBrake(float Value);

    UFUNCTION(Server, Reliable)
    void ServerSetSteering(float Value);

    UFUNCTION(Server, Reliable)
    void ServerShiftGear(int32 Direction);

    ACoopVehicle* GetVehicleOnServer();

    TWeakObjectPtr<ACoopVehicle> CachedVehicle;

    bool bSteerLeftHeld = false;
    bool bSteerRightHeld = false;
};
