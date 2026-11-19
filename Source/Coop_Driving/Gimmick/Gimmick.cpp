#include "Gimmick/Gimmick.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameMode/DrivingGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Vehicle/CoopVehicle.h"

AGimmick::AGimmick()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("GimmickRoot")));
    ActivationZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ActivationZone"));
    ActivationZone->SetupAttachment(RootComponent);
    // Keep the approach area independent of the placeholder body's non-uniform mesh scale.
    ActivationZone->SetAbsolute(false, false, true);
    ActivationZone->SetBoxExtent(FVector(1800.f, 1800.f, 400.f));
    ActivationZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ActivationZone->SetGenerateOverlapEvents(false);
    ActivationZone->ShapeColor = FColor::Cyan;
    DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
    DirectionArrow->SetupAttachment(RootComponent);
    DirectionArrow->SetAbsolute(false, false, true);
    DirectionArrow->ArrowColor = FColor::Cyan;
    DirectionArrow->ArrowSize = 2.f;
}

void AGimmick::SetGimmickRoot(USceneComponent* NewRoot)
{
    USceneComponent* PreviousRoot = GetRootComponent();
    SetRootComponent(NewRoot);
    // Preserve the editor helpers when a hazard uses its physics body as the actor root.
    if (PreviousRoot && PreviousRoot != NewRoot) PreviousRoot->SetupAttachment(NewRoot);
}

void AGimmick::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) bWaitingForVehicle = bActive && ActivationMode == EGimmickActivationMode::VehicleApproach;
    ResetApproachObservation();
    SetActorTickEnabled(HasAuthority() && bActive);
    OnActivationChanged();
}

void AGimmick::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || !bActive) return;
    if (bWaitingForVehicle)
    {
        if (!CheckVehicleApproach()) return;
        bWaitingForVehicle = false;
        OnActivationChanged();
        ForceNetUpdate();
    }
    UpdateGimmick(DeltaSeconds);
}

void AGimmick::SetGimmickActive(bool bNewActive)
{
    if (!HasAuthority() || bActive == bNewActive) return;
    bActive = bNewActive;
    bWaitingForVehicle = bActive && ActivationMode == EGimmickActivationMode::VehicleApproach;
    ResetApproachObservation();
    SetActorTickEnabled(bActive);
    OnActivationChanged();
    ForceNetUpdate();
}

void AGimmick::OnRep_Active()
{
    OnActivationChanged();
}

void AGimmick::SetTargetVehicle(ACoopVehicle* Vehicle)
{
    if (HasAuthority())
    {
        TargetVehicle = Vehicle;
        ResetApproachObservation();
        ForceNetUpdate();
    }
}

ACoopVehicle* AGimmick::GetTargetVehicle() const
{
    if (IsValid(TargetVehicle)) return TargetVehicle;
    const ADrivingGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ADrivingGameMode>() : nullptr;
    if (Mode && IsValid(Mode->GetSharedVehicle())) return Mode->GetSharedVehicle();
    if (!GetWorld() || !HasAuthority()) return nullptr;
    if (AutoDetectedVehicle.IsValid()) return AutoDetectedVehicle.Get();
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextVehicleSearchTime) return nullptr;
    NextVehicleSearchTime = Now + 0.5;
    for (TActorIterator<ACoopVehicle> It(GetWorld()); It; ++It)
    {
        AutoDetectedVehicle = *It;
        return *It;
    }
    return nullptr;
}

void AGimmick::ResetApproachObservation()
{
    ApproachVehicle.Reset();
    bHasApproachLocation = false;
}

bool AGimmick::IsInActivationZone(const FVector& WorldLocation) const
{
    const FVector Local = ActivationZone->GetComponentTransform().InverseTransformPosition(WorldLocation);
    const FVector Extent = ActivationZone->GetUnscaledBoxExtent();
    return FBox(-Extent, Extent).IsInsideOrOn(Local);
}

bool AGimmick::CheckVehicleApproach()
{
    ACoopVehicle* Vehicle = GetTargetVehicle();
    if (!IsTargetVehicle(Vehicle))
    {
        ResetApproachObservation();
        return false;
    }
    if (ApproachVehicle.Get() != Vehicle)
    {
        ApproachVehicle = Vehicle;
        bHasApproachLocation = false;
    }
    const FVector Location = Vehicle->GetActorLocation();
    bool bEntered = IsInActivationZone(Location);
    if (!bEntered && bHasApproachLocation)
    {
        const FTransform Frame = ActivationZone->GetComponentTransform();
        const FVector From = Frame.InverseTransformPosition(PreviousApproachLocation);
        const FVector To = Frame.InverseTransformPosition(Location);
        const FVector Extent = ActivationZone->GetUnscaledBoxExtent();
        bEntered = FMath::LineBoxIntersection(FBox(-Extent, Extent), From, To, To - From);
    }
    PreviousApproachLocation = Location;
    bHasApproachLocation = true;
    return bEntered;
}

bool AGimmick::IsTargetVehicle(const ACoopVehicle* Vehicle) const
{
    return IsValid(Vehicle) && Vehicle == GetTargetVehicle() && !Vehicle->IsDrivingDisabled();
}

void AGimmick::BindVehicleCollision(UPrimitiveComponent* Component)
{
    Component->SetNotifyRigidBodyCollision(true);
    Component->OnComponentHit.AddUniqueDynamic(this, &ThisClass::HandleComponentHit);
}

void AGimmick::HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
    ACoopVehicle* Vehicle = Cast<ACoopVehicle>(OtherActor);
    if (HasAuthority() && bActive && !bWaitingForVehicle && IsTargetVehicle(Vehicle)) OnVehicleHit(Vehicle, Hit);
}

void AGimmick::ApplyEffect(ACoopVehicle* Vehicle)
{
    if (!HasAuthority() || !IsValid(Vehicle)) return;
    OnEffectApplied.Broadcast(Vehicle);
    ReceiveApplyEffect(Vehicle);
    MulticastTriggered(Vehicle);
}

void AGimmick::MulticastTriggered_Implementation(ACoopVehicle* Vehicle)
{
    ReceiveGimmickTriggered(Vehicle);
}

void AGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGimmick, bActive);
    DOREPLIFETIME(AGimmick, TargetVehicle);
    DOREPLIFETIME(AGimmick, bWaitingForVehicle);
}
