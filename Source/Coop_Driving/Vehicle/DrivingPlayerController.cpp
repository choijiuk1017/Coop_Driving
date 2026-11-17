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
}

void ADrivingPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) RandomizeGearPatterns();
    if (IsLocalController()) SetInputMode(FInputModeGameOnly());
    RefreshRoleBinding();
    // Read physical key edges below: old IMC assets cannot double-trigger braking or shifts.
}

void ADrivingPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    RefreshRoleBinding();
}

void ADrivingPlayerController::RefreshRoleBinding()
{
    APlayerState* State = GetPlayerState<APlayerState>();
    UPlayerRoleComponent* Component = State ? State->FindComponentByClass<UPlayerRoleComponent>() : nullptr;
    if (BoundRoleComponent.Get() == Component) return;
    if (BoundRoleComponent.IsValid()) BoundRoleComponent->OnAssignmentChanged.RemoveAll(this);
    BoundRoleComponent = Component;
    if (Component)
        Component->OnAssignmentChanged.AddUObject(this, &ThisClass::HandleRoleAssignmentChanged);
    HandleRoleAssignmentChanged();
}

void ADrivingPlayerController::HandleRoleAssignmentChanged()
{
    if (HasAuthority())
    {
        ClearDrivingInput();
        AppliedRole = GetDrivingRole();
    }
    if (IsLocalController())
    {
        InputSendElapsed = 0.1f;
        if (BoundRoleComponent.IsValid()) BoundRoleComponent->RefreshLocalVoiceEffects();
    }
    UE_LOG(LogTemp, Log, TEXT("[Driving][%s] Player=%s Role=%s Vehicle=%s"),
        HasAuthority() ? TEXT("Server") : TEXT("Client"), *GetNameSafe(PlayerState),
        *UEnum::GetValueAsString(GetDrivingRole()), *GetNameSafe(SharedVehicle));
}

void ADrivingPlayerController::OnRep_SharedVehicle()
{
    RefreshRoleBinding();
    if (IsLocalController() && IsValid(SharedVehicle)) SetViewTarget(SharedVehicle);
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
    RefreshRoleBinding();
    if (SharedVehicle == Vehicle) return;
    ClearDrivingInput();
    SharedVehicle = Vehicle;
    OnRep_SharedVehicle();
    HandleRoleAssignmentChanged();
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
    GearFeedback.Reset();
    LastDrivingPacket = -1.0;
}

void ADrivingPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BoundRoleComponent.IsValid()) BoundRoleComponent->OnAssignmentChanged.RemoveAll(this);
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
            GearFeedback = TEXT("Timed out - new patterns");
            RandomizeGearPatterns();
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
        if (WasInputKeyJustPressed(EKeys::Up)) ServerGearArrow(EGearArrow::Up, PatternRevision);
        if (WasInputKeyJustPressed(EKeys::Down)) ServerGearArrow(EGearArrow::Down, PatternRevision);
        if (WasInputKeyJustPressed(EKeys::Left)) ServerGearArrow(EGearArrow::Left, PatternRevision);
        if (WasInputKeyJustPressed(EKeys::Right)) ServerGearArrow(EGearArrow::Right, PatternRevision);
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

void ADrivingPlayerController::ServerGearArrow_Implementation(EGearArrow Arrow, uint32 ExpectedRevision)
{
    if (GetDrivingRole() != EDrivingRole::Gear || !IsValid(SharedVehicle)
        || static_cast<uint8>(Arrow) > static_cast<uint8>(EGearArrow::Right)) return;
    if (AppliedRole != EDrivingRole::Gear)
    {
        ClearDrivingInput();
        AppliedRole = EDrivingRole::Gear;
    }
    if (ExpectedRevision != PatternRevision) return;
    if (GearState.Expire(GetWorld()->GetTimeSeconds(), GearInputTimeout))
    {
        GearFeedback = TEXT("Timed out - new patterns");
        RandomizeGearPatterns();
        return;
    }
    const int32 Result = GearState.Push(Arrow, GetWorld()->GetTimeSeconds(), GearInputTimeout,
        ShiftUpPattern, ShiftDownPattern, ReversePattern);
    GearProgress = GearState.Entered;
    if (Result == 1 || Result == -1 || Result == 3)
    {
        const int32 Before = SharedVehicle->GetSelectedGear();
        const int32 Target = Result == 3 ? (Before < 0 ? 1 : -1) : Before + Result;
        SharedVehicle->ChangeGear(Target);
        GearFeedback = Before == SharedVehicle->GetSelectedGear()
            ? TEXT("Shift blocked: slow down / check gear limit")
            : SharedVehicle->GetSelectedGear() < 0 ? TEXT("REVERSE (R) - pedals: W to reverse") : TEXT("Shift complete");
    }
    else GearFeedback = Result == 2 ? TEXT("Wrong input - new patterns") : TEXT("Entering pattern...");
    if (Result != 0) RandomizeGearPatterns();
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
        return FString::Printf(TEXT("GEAR | UP: %s\nDOWN: %s\nR / D: %s\nInput: %s\n%s"),
            *PatternText(ShiftUpPattern), *PatternText(ShiftDownPattern), *PatternText(ReversePattern), *PatternText(GearProgress), *GearFeedback);
    default: return TEXT("Waiting for all 4 players and role assignment...");
    }
}

void ADrivingPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADrivingPlayerController, SharedVehicle);
    DOREPLIFETIME(ADrivingPlayerController, ShiftUpPattern);
    DOREPLIFETIME(ADrivingPlayerController, ShiftDownPattern);
    DOREPLIFETIME(ADrivingPlayerController, ReversePattern);
    DOREPLIFETIME(ADrivingPlayerController, PatternRevision);
    DOREPLIFETIME(ADrivingPlayerController, GearProgress);
    DOREPLIFETIME(ADrivingPlayerController, GearFeedback);
}

void ADrivingPlayerController::RandomizeGearPatterns()
{
    if (!HasAuthority()) return;
    TArray<EGearArrow> Starts = { EGearArrow::Up, EGearArrow::Down, EGearArrow::Left, EGearArrow::Right };
    for (int32 Index = Starts.Num() - 1; Index > 0; --Index)
        Starts.Swap(Index, FMath::RandRange(0, Index));
    TArray<EGearArrow>* Patterns[] = { &ShiftUpPattern, &ShiftDownPattern, &ReversePattern };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const TArray<EGearArrow> Previous = *Patterns[Index];
        TArray<EGearArrow>& Pattern = *Patterns[Index];
        Pattern = { Starts[Index] };
        for (int32 Step = 1; Step < 4; ++Step)
            Pattern.Add(static_cast<EGearArrow>(FMath::RandRange(0, 3)));
        if (Pattern == Previous)
            Pattern.Last() = static_cast<EGearArrow>((static_cast<uint8>(Pattern.Last()) + 1) % 4);
    }
    GearState = FGearPatternState();
    GearProgress.Reset();
    ++PatternRevision;
    ForceNetUpdate();
}
