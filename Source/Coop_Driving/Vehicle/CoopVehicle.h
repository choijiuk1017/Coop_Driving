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

protected:
	virtual void BeginPlay() override;
	
};
