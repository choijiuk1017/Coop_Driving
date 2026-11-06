// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/CoopVehicle.h"

ACoopVehicle::ACoopVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);
}

void ACoopVehicle::BeginPlay()
{
	Super::BeginPlay();


}