#include "Actor/DrivingDestinationZone.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameMode/DrivingGameMode.h"
#include "TimerManager.h"
#include "Vehicle/CoopVehicle.h"

ADrivingDestinationZone::ADrivingDestinationZone()
{
    PrimaryActorTick.bCanEverTick = false;
    DestinationBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("DestinationBounds"));
    SetRootComponent(DestinationBounds);
    DestinationBounds->SetBoxExtent(FVector(400.f, 400.f, 300.f));
    DestinationBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DestinationBounds->SetGenerateOverlapEvents(false);
    DestinationBounds->ShapeColor = FColor::Green;
}

void ADrivingDestinationZone::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
        GetWorldTimerManager().SetTimer(CheckTimer, this, &ThisClass::CheckDestination, 0.1f, true);
}

bool ADrivingDestinationZone::ContainsLocation(const FVector& WorldLocation) const
{
    const FVector Local = DestinationBounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
    const FVector Extent = DestinationBounds->GetUnscaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y
        && FMath::Abs(Local.Z) <= Extent.Z;
}

void ADrivingDestinationZone::CheckDestination()
{
    if (!HasAuthority()) return;
    ADrivingGameMode* Mode = GetWorld()->GetAuthGameMode<ADrivingGameMode>();
    ACoopVehicle* Vehicle = Mode ? Mode->GetSharedVehicle() : nullptr;
    if (IsValid(Vehicle) && ContainsLocation(Vehicle->GetActorLocation()) && Mode->TryFinishDriving(Vehicle))
        GetWorldTimerManager().ClearTimer(CheckTimer);
}

void ADrivingDestinationZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(CheckTimer);
    Super::EndPlay(EndPlayReason);
}
