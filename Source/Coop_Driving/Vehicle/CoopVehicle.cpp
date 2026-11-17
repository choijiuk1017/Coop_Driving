// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/CoopVehicle.h"
#include "ChaosVehicleMovementComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Vehicle/PhysicsCargo.h"
#include "Vehicle/CargoSpeedBump.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

ACoopVehicle::ACoopVehicle()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true; // Empty player pawns stay at spawn while the vehicle moves away.
	SetReplicateMovement(true);
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
    CargoDeck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoDeck"));
    CargoDeck->SetupAttachment(GetMesh());
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DeckMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    CargoDeck->SetStaticMesh(DeckMesh.Object);
    CargoDeck->SetRelativeLocation(FVector(-140.f, 0.f, 110.f));
    CargoDeck->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.12f));
    CargoDeck->SetCollisionProfileName(TEXT("BlockAll"));
    CargoDeck->BodyInstance.bAutoWeld = true;
    CargoClass = APhysicsCargo::StaticClass();
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
    if (bSpawnCargo && CargoClass)
    {
        for (int32 Index = 0; Index < FMath::Clamp(CargoCount, 0, 12); ++Index)
        {
            const FVector Offset(-30.f + (Index % 2) * 60.f, -30.f + ((Index / 2) % 2) * 60.f,
                32.f + (Index / 4) * 48.f);
            const FVector Location = CargoDeck->GetComponentLocation() + CargoDeck->GetComponentQuat().RotateVector(Offset);
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
            APhysicsCargo* Cargo = GetWorld()->SpawnActor<APhysicsCargo>(CargoClass, Location, CargoDeck->GetComponentRotation(), Params);
            if (Cargo) Cargo->Body->SetPhysicsLinearVelocity(GetVelocity());
            else UE_LOG(LogTemp, Warning, TEXT("[Cargo] Box %d blocked. Adjust CargoDeck position in the vehicle BP."), Index);
        }
    }
    if (bSpawnTestSpeedBump)
    {
        const FVector Ahead = GetActorLocation() + GetActorForwardVector() * TestBumpDistance;
        FHitResult Ground;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(CargoBumpGround), false, this);
        if (GetWorld()->LineTraceSingleByObjectType(Ground, Ahead + FVector(0,0,500), Ahead - FVector(0,0,2000),
            FCollisionObjectQueryParams(ECC_WorldStatic), Query))
        {
            GetWorld()->SpawnActor<ACargoSpeedBump>(Ground.ImpactPoint,
                FRotator(0.f, GetActorRotation().Yaw, 0.f));
        }
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
	// cm/s: avoid engaging the opposite direction while travelling faster than 3.6 km/h.
	const float ForwardSpeed = Movement->GetForwardSpeed();
	if ((Gear < 0 && ForwardSpeed > 100.f) || (Gear > 0 && ForwardSpeed < -100.f)) return;
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
