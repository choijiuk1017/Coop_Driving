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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	int32 GetSelectedGear() const { return SelectedGear; }
	float GetBrakePressure() const { return BrakePressure; }

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

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	int32 SelectedGear = 1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	float BrakePressure = 0.f;
	
};
