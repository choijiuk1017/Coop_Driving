#include "Vehicle/PhysicsCargo.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
APhysicsCargo::APhysicsCargo()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoBody"));
    SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Cube.Object);
    Body->SetRelativeScale3D(FVector(0.45f));
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));
    Body->SetSimulatePhysics(true);
    Body->SetUseCCD(true);
    Body->SetLinearDamping(0.1f);
    Body->SetAngularDamping(0.3f);
    NetUpdateFrequency = 60.f;
    MinNetUpdateFrequency = 20.f;
}
void APhysicsCargo::BeginPlay()
{
    Super::BeginPlay();
    Body->SetMassOverrideInKg(NAME_None, MassKg);
    // Simulated proxies receive authoritative rigid-body movement from the server.
}
