// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/CoopVehicle.h"
#include "ChaosVehicleMovementComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

ACoopVehicle::ACoopVehicle()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);
}

void ACoopVehicle::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    UChaosVehicleMovementComponent* Movement = GetVehicleMovementComponent();

    if (Movement)
    {
        Movement->SetRequiresControllerForInputs(false);
    }
}

void ACoopVehicle::SetThrottle(float Value)
{
	if (!HasAuthority()) return;

	GetVehicleMovementComponent()->SetThrottleInput(FMath::Clamp(Value, 0.f, 1.f));
}

void ACoopVehicle::SetBrake(float Value)
{
	if (!HasAuthority()) return;

	GetVehicleMovementComponent()->SetBrakeInput(FMath::Clamp(Value, 0.f, 1.f));
}

void ACoopVehicle::SetSteering(float Value)
{
	if (!HasAuthority()) return;

	GetVehicleMovementComponent()->SetSteeringInput(FMath::Clamp(Value, -1.f, 1.f));
}

void ACoopVehicle::ChangeGear(int32 Gear)
{
	if (!HasAuthority()) return;

	GetVehicleMovementComponent()->SetUseAutomaticGears(false);
	GetVehicleMovementComponent()->SetTargetGear(Gear, true);
}