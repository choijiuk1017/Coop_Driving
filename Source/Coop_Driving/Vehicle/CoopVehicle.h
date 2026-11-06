// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "CoopVehicle.generated.h"

/**
 * 
 */
UCLASS()
class COOP_DRIVING_API ACoopVehicle : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	ACoopVehicle();

	UFUNCTION(BlueprintCallable, Category = "Vehicle") 
	void SetThrottle(float Value);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void SetBrake(float Value);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void SetSteering(float Value);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	void ChangeGear(int32 Gear);

protected:
	virtual void BeginPlay() override;
	
};
