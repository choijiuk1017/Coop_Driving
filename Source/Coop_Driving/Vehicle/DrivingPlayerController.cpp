#include "Vehicle/DrivingPlayerController.h"
#include "Vehicle/CoopVehicle.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "InputCoreTypes.h"

namespace
{
    FString PatternText(const TArray<EGearArrow>& Pattern)
    {
        FString Text;
        for (EGearArrow Arrow : Pattern)
        {
            if (!Text.IsEmpty()) Text += TEXT(" ");
            switch (Arrow)
            {
            case EGearArrow::Up: Text += TEXT("UP"); break;
            case EGearArrow::Down: Text += TEXT("DOWN"); break;
            case EGearArrow::Left: Text += TEXT("LEFT"); break;
            case EGearArrow::Right: Text += TEXT("RIGHT"); break;
            }
        }
        return Text;
    }
}

ADrivingPlayerController::ADrivingPlayerController()
{
    bShowMouseCursor = false;
    bAutoManageActiveCameraTarget = false;
    ShiftUpPattern = { EGearArrow::Up, EGearArrow::Right, EGearArrow::Down, EGearArrow::Up };
    ShiftDownPattern = { EGearArrow::Down, EGearArrow::Left, EGearArrow::Up, EGearArrow::Down };
}

void ADrivingPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController()) SetInputMode(FInputModeGameOnly());
    // Read physical key edges below: old IMC assets cannot double-trigger braking or shifts.
}

EDrivingRole ADrivingPlayerController::GetDrivingRole() const
{
    const APlayerState* State = GetPlayerState<APlayerState>();
    const UPlayerRoleComponent* DrivingRole = State ? State->FindComponentByClass<UPlayerRoleComponent>() : nullptr;
    return DrivingRole ? DrivingRole->GetRole() : EDrivingRole::None;
}

void ADrivingPlayerController::SetSharedVehicle(ACoopVehicle* Vehicle)
{
    if (!HasAuthority()) return;
    ClearDrivingInput();
    SharedVehicle = Vehicle;
    ForceNetUpdate();
}

void ADrivingPlayerController::ClearDrivingInput()
{
    if (!HasAuthority()) return;
    if (IsValid(SharedVehicle))
    {
        if (AppliedRole == EDrivingRole::Pedals)
        {
            SharedVehicle->SetThrottle(0.f);
            SharedVehicle->SetBrake(0.f);
        }
        if (AppliedRole == EDrivingRole::Steering) SharedVehicle->SetSteering(0.f);
    }
    AppliedRole = EDrivingRole::None;
    BrakeState = FBrakeTapState();
    GearState = FGearPatternState();
    GearProgress.Reset();
    LastDrivingPacket = -1.0;
}

void ADrivingPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearDrivingInput();
    Super::EndPlay(EndPlayReason);
}

void ADrivingPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const EDrivingRole DrivingRole = GetDrivingRole();
    if (HasAuthority())
    {
        if (AppliedRole != DrivingRole)
        {
            ClearDrivingInput();
            AppliedRole = DrivingRole;
        }
        const double Now = GetWorld()->GetTimeSeconds();
        if (IsValid(SharedVehicle))
        {
            if (LastDrivingPacket < 0.0 || Now - LastDrivingPacket > 0.5)
            {
                if (DrivingRole == EDrivingRole::Pedals)
                {
                    SharedVehicle->SetThrottle(0.f);
                    BrakeState = FBrakeTapState();
                }
                if (DrivingRole == EDrivingRole::Steering) SharedVehicle->SetSteering(0.f);
            }
            if (DrivingRole == EDrivingRole::Pedals)
            {
                BrakeState.Decay(DeltaTime, BrakeDecayPerSecond);
                SharedVehicle->SetBrake(BrakeState.Pressure);
            }
        }
        if (GearState.Expire(Now, GearInputTimeout))
        {
            GearProgress.Reset();
            GearFeedback = TEXT("Timed out - try again");
        }
    }

}

void ADrivingPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    const EDrivingRole DrivingRole = GetDrivingRole();
    if (!IsLocalController()) return;
    if (IsValid(SharedVehicle) && GetViewTarget() != SharedVehicle) SetViewTarget(SharedVehicle);

    const bool bThrottle = DrivingRole == EDrivingRole::Pedals && IsInputKeyDown(EKeys::W);
    const float Steering = DrivingRole == EDrivingRole::Steering
        ? (IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (IsInputKeyDown(EKeys::A) ? 1.f : 0.f) : 0.f;
    InputSendElapsed += DeltaTime;
    if (InputSendElapsed >= 0.1f || bThrottle != bPreviousThrottle || Steering != PreviousSteering)
    {
        ServerDrivingState(bThrottle, Steering);
        InputSendElapsed = 0.f;
        bPreviousThrottle = bThrottle;
        PreviousSteering = Steering;
    }
    if (DrivingRole == EDrivingRole::Pedals && WasInputKeyJustPressed(EKeys::SpaceBar)) ServerBrakeTap();
    if (DrivingRole == EDrivingRole::Gear)
    {
        if (WasInputKeyJustPressed(EKeys::Up)) ServerGearArrow(EGearArrow::Up);
        if (WasInputKeyJustPressed(EKeys::Down)) ServerGearArrow(EGearArrow::Down);
        if (WasInputKeyJustPressed(EKeys::Left)) ServerGearArrow(EGearArrow::Left);
        if (WasInputKeyJustPressed(EKeys::Right)) ServerGearArrow(EGearArrow::Right);
    }
}

void ADrivingPlayerController::ServerDrivingState_Implementation(bool bThrottle, float Steering)
{
    const EDrivingRole DrivingRole = GetDrivingRole();
    if (!IsValid(SharedVehicle)) return;
    if (AppliedRole != DrivingRole)
    {
        ClearDrivingInput();
        AppliedRole = DrivingRole;
    }
    LastDrivingPacket = GetWorld()->GetTimeSeconds();
    if (DrivingRole == EDrivingRole::Pedals) SharedVehicle->SetThrottle(bThrottle ? 1.f : 0.f);
    if (DrivingRole == EDrivingRole::Steering && FMath::IsFinite(Steering))
        SharedVehicle->SetSteering(FMath::Clamp(Steering, -1.f, 1.f));
}

void ADrivingPlayerController::ServerBrakeTap_Implementation()
{
    if (GetDrivingRole() != EDrivingRole::Pedals || !IsValid(SharedVehicle)) return;
    if (AppliedRole != EDrivingRole::Pedals)
    {
        ClearDrivingInput();
        AppliedRole = EDrivingRole::Pedals;
    }
    BrakeState.Tap(GetWorld()->GetTimeSeconds(), BrakeTapWindow, BrakeGainPerTap);
    SharedVehicle->SetBrake(BrakeState.Pressure);
}

void ADrivingPlayerController::ServerGearArrow_Implementation(EGearArrow Arrow)
{
    if (GetDrivingRole() != EDrivingRole::Gear || !IsValid(SharedVehicle)
        || static_cast<uint8>(Arrow) > static_cast<uint8>(EGearArrow::Right)) return;
    if (AppliedRole != EDrivingRole::Gear)
    {
        ClearDrivingInput();
        AppliedRole = EDrivingRole::Gear;
    }
    const int32 Result = GearState.Push(Arrow, GetWorld()->GetTimeSeconds(), GearInputTimeout,
        ShiftUpPattern, ShiftDownPattern);
    GearProgress = GearState.Entered;
    if (Result == 1 || Result == -1)
    {
        const int32 Before = SharedVehicle->GetSelectedGear();
        SharedVehicle->ChangeGear(Before + Result);
        GearFeedback = Before == SharedVehicle->GetSelectedGear() ? TEXT("Gear limit reached") : TEXT("Shift complete");
    }
    else GearFeedback = Result == 2 ? TEXT("Wrong input - try again") : TEXT("Entering pattern...");
    ForceNetUpdate();
}

FString ADrivingPlayerController::GetDrivingHelp() const
{
    switch (GetDrivingRole())
    {
    case EDrivingRole::Steering: return TEXT("STEERING | A / D: turn");
    case EDrivingRole::Pedals: return TEXT("PEDALS | Hold W: throttle | Repeated SPACE taps: brake");
    case EDrivingRole::Briefing: return TEXT("BRIEFING | Guide your team. Driving controls are disabled.");
    case EDrivingRole::Gear:
        return FString::Printf(TEXT("GEAR | UP: %s\nDOWN: %s\nInput: %s\n%s"),
            *PatternText(ShiftUpPattern), *PatternText(ShiftDownPattern), *PatternText(GearProgress), *GearFeedback);
    default: return TEXT("Waiting for all 4 players and role assignment...");
    }
}

void ADrivingPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADrivingPlayerController, SharedVehicle);
    DOREPLIFETIME(ADrivingPlayerController, GearProgress);
    DOREPLIFETIME(ADrivingPlayerController, GearFeedback);
}
