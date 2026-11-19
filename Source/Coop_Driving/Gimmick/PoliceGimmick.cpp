#include "Gimmick/PoliceGimmick.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameMode/DrivingGameMode.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicle/CoopVehicle.h"

APoliceGimmick::APoliceGimmick()
{
    ActivationMode = EGimmickActivationMode::VehicleApproach;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PoliceBody"));
    SetGimmickRoot(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(4.f, 1.8f, 1.f));
    Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    BindVehicleCollision(Body);
    SetNetUpdateFrequency(30.f);
}

void APoliceGimmick::OnActivationChanged()
{
    const bool bPursuing = bActive && !IsWaitingForVehicle();
    Body->SetVisibility(bPursuing);
    Body->SetCollisionEnabled(bPursuing ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (!HasAuthority()) return;
    if (bActive)
    {
        bCaughtVehicle = false;
        bPursuitStarted = false;
        PursuitElapsed = 0.f;
    }
}

FVector APoliceGimmick::GroundedPosition(const FVector& Position, ACoopVehicle* Vehicle) const
{
    FHitResult Ground;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PoliceRoad), false, this);
    Query.AddIgnoredActor(Vehicle);
    if (GetWorld()->LineTraceSingleByObjectType(Ground, Position + FVector(0.f, 0.f, 400.f),
        Position - FVector(0.f, 0.f, 1500.f), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
    {
        FVector Result = Position;
        Result.Z = Ground.ImpactPoint.Z + Body->Bounds.BoxExtent.Z + 5.f;
        return Result;
    }
    return Position;
}

void APoliceGimmick::UpdateGimmick(float DeltaSeconds)
{
    ACoopVehicle* Vehicle = GetTargetVehicle();
    if (!IsTargetVehicle(Vehicle))
    {
        if (bPursuitStarted) SetGimmickActive(false);
        return;
    }
    if (!bPursuitStarted)
    {
        bPursuitStarted = true;
        if (bStartBehindVehicle)
        {
            const FVector Start = Vehicle->GetActorLocation()
                - Vehicle->GetActorForwardVector().GetSafeNormal2D() * FMath::Max(StartDistanceBehind, CatchDistance + 500.f);
            SetActorLocationAndRotation(GroundedPosition(Start, Vehicle), FRotator(0.f, Vehicle->GetActorRotation().Yaw, 0.f),
                false, nullptr, ETeleportType::TeleportPhysics);
        }
    }

    const FVector ToVehicle = Vehicle->GetActorLocation() - GetActorLocation();
    if (ToVehicle.Size() <= CatchDistance)
    {
        CatchVehicle(Vehicle);
        return;
    }
    PursuitElapsed += DeltaSeconds;
    if (PursuitElapsed >= FMath::Max(0.1f, PursuitDuration))
    {
        SetGimmickActive(false);
        return;
    }

    const FVector Direction = ToVehicle.GetSafeNormal2D();
    const float Step = FMath::Min(FMath::Max(0.f, PursuitSpeed) * DeltaSeconds, ToVehicle.Size2D());
    const FVector Next = GroundedPosition(GetActorLocation() + Direction * Step, Vehicle);
    SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
    FHitResult Hit;
    SetActorLocation(Next, true, &Hit);
    // Swept contacts can occur even if the vehicle has rigid-body hit notifications disabled.
    if (bActive && (Hit.GetActor() == Vehicle || FVector::Dist(GetActorLocation(), Vehicle->GetActorLocation()) <= CatchDistance))
        CatchVehicle(Vehicle);
}

void APoliceGimmick::OnVehicleHit(ACoopVehicle* Vehicle, const FHitResult& Hit)
{
    if (bPursuitStarted) CatchVehicle(Vehicle);
}

void APoliceGimmick::CatchVehicle(ACoopVehicle* Vehicle)
{
    if (!HasAuthority() || !bActive || bCaughtVehicle || !IsTargetVehicle(Vehicle)) return;
    bCaughtVehicle = true;
    SetGimmickActive(false);
    ApplyEffect(Vehicle);
}

void APoliceGimmick::ApplyEffect(ACoopVehicle* Vehicle)
{
    if (bGameOverOnCatch)
    {
        if (ADrivingGameMode* Mode = GetWorld()->GetAuthGameMode<ADrivingGameMode>())
            Mode->TryFailDriving(Vehicle, this);
        else
            Vehicle->DisableDriving(); // Also functional in a standalone vehicle test map.
    }
    // Alternative penalty examples (future inventory/wallet integration):
    // CargoInventory->RemoveCargo(CargoPenalty);
    // Wallet->SpendCurrency(CurrencyPenalty);
    Super::ApplyEffect(Vehicle);
}

void APoliceGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APoliceGimmick, bCaughtVehicle);
}
