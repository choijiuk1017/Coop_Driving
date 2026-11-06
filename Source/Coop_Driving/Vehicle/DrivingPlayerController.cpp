// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/DrivingPlayerController.h"

#include "Vehicle/CoopVehicle.h"
#include "ChaosVehicleMovementComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ADrivingPlayerController::ADrivingPlayerController()
{
    bShowMouseCursor = false;
}

void ADrivingPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

    if (Subsystem && DrivingMappingContext)
    {
        Subsystem->AddMappingContext(DrivingMappingContext, 0);
    }
}

void ADrivingPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);

    if (!Input) return;

    if (ThrottleAction)
    {
        Input->BindAction(ThrottleAction, ETriggerEvent::Started, this, &ADrivingPlayerController::PressThrottle);

        Input->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ADrivingPlayerController::ReleaseThrottle);

        Input->BindAction(ThrottleAction, ETriggerEvent::Canceled, this, &ADrivingPlayerController::ReleaseThrottle);
    }

    if (BrakeAction)
    {
        Input->BindAction(BrakeAction, ETriggerEvent::Started, this, &ADrivingPlayerController::PressBrake);

        Input->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ADrivingPlayerController::ReleaseBrake);

        Input->BindAction(BrakeAction, ETriggerEvent::Canceled, this, &ADrivingPlayerController::ReleaseBrake);
    }

    if (SteerLeftAction)
    {
        Input->BindAction(SteerLeftAction, ETriggerEvent::Started, this, &ADrivingPlayerController::PressSteerLeft);

        Input->BindAction(SteerLeftAction, ETriggerEvent::Completed, this, &ADrivingPlayerController::ReleaseSteerLeft);

        Input->BindAction(SteerLeftAction, ETriggerEvent::Canceled, this, &ADrivingPlayerController::ReleaseSteerLeft);
    }

    if (SteerRightAction)
    {
        Input->BindAction(SteerRightAction, ETriggerEvent::Started, this, &ADrivingPlayerController::PressSteerRight);

        Input->BindAction(SteerRightAction, ETriggerEvent::Completed, this, &ADrivingPlayerController::ReleaseSteerRight);

        Input->BindAction(SteerRightAction, ETriggerEvent::Canceled, this, &ADrivingPlayerController::ReleaseSteerRight);
    }

    if (GearUpAction)
    {
        Input->BindAction(GearUpAction, ETriggerEvent::Started, this, &ADrivingPlayerController::GearUp);
    }

    if (GearDownAction)
    {
        Input->BindAction(GearDownAction, ETriggerEvent::Started, this, &ADrivingPlayerController::GearDown);
    }
}

void ADrivingPlayerController::PressThrottle()
{
    ServerSetThrottle(1.f);

    UE_LOG(LogTemp, Warning, TEXT("[INPUT] W Pressed"));
}

void ADrivingPlayerController::ReleaseThrottle()
{
    ServerSetThrottle(0.f);
}

void ADrivingPlayerController::PressBrake()
{
    ServerSetBrake(1.f);
    UE_LOG(LogTemp, Warning, TEXT("[INPUT] SPace Pressed"));
}

void ADrivingPlayerController::ReleaseBrake()
{
    ServerSetBrake(0.f);
}

void ADrivingPlayerController::PressSteerLeft()
{
    bSteerLeftHeld = true;
    UpdateSteering();
    UE_LOG(LogTemp, Warning, TEXT("[INPUT] A Pressed"));
}

void ADrivingPlayerController::ReleaseSteerLeft()
{
    bSteerLeftHeld = false;
    UpdateSteering();
}

void ADrivingPlayerController::PressSteerRight()
{
    bSteerRightHeld = true;
    UpdateSteering();

    UE_LOG(LogTemp, Warning, TEXT("[INPUT] D Pressed"));
}

void ADrivingPlayerController::ReleaseSteerRight()
{
    bSteerRightHeld = false;
    UpdateSteering();
}

void ADrivingPlayerController::UpdateSteering()
{
    const float SteeringValue = (bSteerRightHeld ? 1.f : 0.f) - (bSteerLeftHeld ? 1.f : 0.f);

    ServerSetSteering(SteeringValue);
}

void ADrivingPlayerController::GearUp()
{
    ServerShiftGear(1);
    UE_LOG(LogTemp, Warning, TEXT("[INPUT] Up Pressed"));

}

void ADrivingPlayerController::GearDown()
{
    ServerShiftGear(-1);

    UE_LOG(LogTemp, Warning, TEXT("[INPUT] Down Pressed"));
}

ACoopVehicle* ADrivingPlayerController::GetVehicleOnServer()
{
    if (!HasAuthority())
    {
        return nullptr;
    }

    if (ACoopVehicle* Vehicle = Cast<ACoopVehicle>(GetPawn()))
    {
        return Vehicle;
    }

    if (CachedVehicle.IsValid())
    {
        return CachedVehicle.Get();
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ACoopVehicle> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            CachedVehicle = *It;
            return *It;
        }
    }

    return nullptr;
}

void ADrivingPlayerController::ServerSetThrottle_Implementation(float Value)
{
    if (ACoopVehicle* Vehicle = GetVehicleOnServer())
    {

        UE_LOG(LogTemp, Warning,
            TEXT("[SERVER] Throttle: %.2f | Vehicle: %s"),
            Value,
            *GetNameSafe(Vehicle));

        Vehicle->SetThrottle(FMath::Clamp(Value, 0.f, 1.f));
    }
}

void ADrivingPlayerController::ServerSetBrake_Implementation(float Value)
{
    if (ACoopVehicle* Vehicle = GetVehicleOnServer())
    {
        Vehicle->SetBrake(FMath::Clamp(Value, 0.f, 1.f));
    }
}

void ADrivingPlayerController::ServerSetSteering_Implementation(float Value)
{
    if (ACoopVehicle* Vehicle = GetVehicleOnServer())
    {
        Vehicle->SetSteering(FMath::Clamp(Value, -1.f, 1.f));
    }
}

void ADrivingPlayerController::ServerShiftGear_Implementation(int32 Direction)
{
    if (Direction != 1 && Direction != -1)
    {
        return;
    }

    ACoopVehicle* Vehicle = GetVehicleOnServer();

    if (!Vehicle)
    {
        return;
    }

    UChaosVehicleMovementComponent* Movement = Vehicle->GetVehicleMovementComponent();

    if (!Movement)
    {
        return;
    }

    const int32 CurrentGear = Movement->GetTargetGear();

    const int32 NewGear = FMath::Clamp(CurrentGear + Direction, -1, MaxForwardGear);

    Vehicle->ChangeGear(NewGear);
}