#include "Gimmick/DeerGimmick.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ADeerGimmick::ADeerGimmick()
{
    ActivationMode = EGimmickActivationMode::VehicleApproach;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeerBody"));
    SetGimmickRoot(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(1.2f, 0.45f, 1.f));
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));
    // Remain at the placed roadside location until the shared vehicle approaches.
    Body->SetSimulatePhysics(false);
    Body->SetUseCCD(true);
    Body->BodyInstance.bLockXRotation = true;
    Body->BodyInstance.bLockYRotation = true;
    Body->BodyInstance.bLockZRotation = true;
    BindVehicleCollision(Body);
    SetNetUpdateFrequency(60.f);
}

void ADeerGimmick::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) Body->SetMassOverrideInKg(NAME_None, FMath::Max(1.f, MassKg));
}

void ADeerGimmick::OnActivationChanged()
{
    if (bConsumed) return; // Preserve the final contact through the delayed destruction.
    const bool bCrossing = bActive && !IsWaitingForVehicle();
    Body->SetVisibility(bCrossing);
    Body->SetCollisionEnabled(bCrossing ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Body->SetSimulatePhysics(bCrossing);
    if (!HasAuthority()) return;
    if (bCrossing)
    {
        CrossingStart = GetActorLocation();
        CrossingDirection = GetActorForwardVector().GetSafeNormal2D();
        ActiveElapsed = 0.f;
        Body->SetPhysicsLinearVelocity(CrossingDirection * CrossingSpeed);
    }
}

void ADeerGimmick::UpdateGimmick(float DeltaSeconds)
{
    if (bConsumed) return;
    ActiveElapsed += DeltaSeconds;
    if (FVector::DotProduct(GetActorLocation() - CrossingStart, CrossingDirection) >= CrossingDistance
        || ActiveElapsed >= FMath::Max(1.f, CrossingDistance / FMath::Max(1.f, CrossingSpeed)) + 5.f)
    {
        Destroy();
        return;
    }
    FVector Velocity = CrossingDirection * FMath::Max(1.f, CrossingSpeed);
    Velocity.Z = Body->GetPhysicsLinearVelocity().Z;
    Body->SetPhysicsLinearVelocity(Velocity);
}

void ADeerGimmick::OnVehicleHit(ACoopVehicle* Vehicle, const FHitResult& Hit)
{
    if (bConsumed) return;
    bConsumed = true;
    ApplyEffect(Vehicle);
    SetGimmickActive(false);
    // Allow the current physics step/contact impulse to finish before removing the rigid body.
    SetLifeSpan(0.05f);
}
