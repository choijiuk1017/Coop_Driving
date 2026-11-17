#include "Vehicle/CargoSpeedBump.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
ACargoSpeedBump::ACargoSpeedBump()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BumpSurface"));
    SetRootComponent(Surface);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Surface->SetStaticMesh(Sphere.Object);
    // Centre on the ground: the exposed upper half is a rounded 15 cm bump.
    Surface->SetRelativeScale3D(FVector(1.6f, 6.f, 0.3f));
    Surface->SetCollisionProfileName(TEXT("BlockAll"));
}
