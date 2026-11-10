// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/CoopVehicle.h"
#include "ChaosVehicleMovementComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

ACoopVehicle::ACoopVehicle()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true; // Empty player pawns stay at spawn while the vehicle moves away.
	SetReplicateMovement(true);
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
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
        Movement->SetUseAutomaticGears(false);
        Movement->bReverseAsBrake = false;
        Movement->bThrottleAsBrake = false;
        Movement->SetThrottleInput(0.f);
        Movement->SetBrakeInput(0.f);
        Movement->SetHandbrakeInput(false);
        ChangeGear(1);
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

	BrakePressure = FMath::Clamp(Value, 0.f, 1.f);
	GetVehicleMovementComponent()->SetBrakeInput(BrakePressure);
}

void ACoopVehicle::SetSteering(float Value)
{
	if (!HasAuthority()) return;

	GetVehicleMovementComponent()->SetSteeringInput(FMath::Clamp(Value, -1.f, 1.f));
}

void ACoopVehicle::ChangeGear(int32 Gear)
{
	if (!HasAuthority()) return;

	UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
	if (!Movement) return;
	const int32 MinGear = Movement->TransmissionSetup.ReverseGearRatios.IsEmpty() ? 0 : -1;
	SelectedGear = FMath::Clamp(Gear, MinGear, Movement->TransmissionSetup.ForwardGearRatios.Num());
	Movement->SetUseAutomaticGears(false);
	Movement->SetTargetGear(SelectedGear, true);
}

void ACoopVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACoopVehicle, SelectedGear);
    DOREPLIFETIME(ACoopVehicle, BrakePressure);
}
