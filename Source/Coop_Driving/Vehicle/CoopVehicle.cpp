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
        return;

    UChaosWheeledVehicleMovementComponent* Movement =
        Cast<UChaosWheeledVehicleMovementComponent>(
            GetVehicleMovementComponent());

    if (!Movement)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[VEHICLE] Invalid Movement Component"));
        return;
    }

    Movement->SetRequiresControllerForInputs(false);

    // Temporary manual gear test
    Movement->SetUseAutomaticGears(false);
    Movement->SetTargetGear(1, true);

    Movement->SetBrakeInput(0.f);
    Movement->SetHandbrakeInput(false);
    Movement->SetSleeping(false);

    UE_LOG(LogTemp, Warning,
        TEXT("[VEHICLE] Wheels=%d | Physics=%d | Gear=%d"),
        Movement->WheelSetups.Num(),
        GetMesh()->IsSimulatingPhysics(),
        Movement->GetTargetGear());

    for (int32 i = 0; i < Movement->WheelSetups.Num(); ++i)
    {
        const FChaosWheelSetup& Wheel = Movement->WheelSetups[i];

        UE_LOG(LogTemp, Warning,
            TEXT("[WHEEL %d] Bone=%s | Class=%s"),
            i,
            *Wheel.BoneName.ToString(),
            *GetNameSafe(Wheel.WheelClass.Get()));
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