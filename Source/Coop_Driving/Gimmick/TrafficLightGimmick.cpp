#include "Gimmick/TrafficLightGimmick.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicle/CoopVehicle.h"

ATrafficLightGimmick::ATrafficLightGimmick()
{
    StopZone = CreateDefaultSubobject<UBoxComponent>(TEXT("StopZone"));
    StopZone->SetupAttachment(RootComponent);
    StopZone->SetBoxExtent(FVector(600.f, 600.f, 300.f));
    StopZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StopZone->SetGenerateOverlapEvents(false);
    StopZone->ShapeColor = FColor::Red;
    SignalPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignalPost"));
    SignalPost->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    SignalPost->SetStaticMesh(Cube.Object);
    SignalPost->SetRelativeLocation(FVector(0.f, 700.f, 200.f));
    SignalPost->SetRelativeScale3D(FVector(0.3f, 0.3f, 4.f));
    SignalPost->SetCollisionProfileName(TEXT("NoCollision"));
    RedLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RedLight"));
    YellowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("YellowLight"));
    GreenLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GreenLight"));
    UPointLightComponent* Lights[] = { RedLight.Get(), YellowLight.Get(), GreenLight.Get() };
    const FLinearColor Colors[] = { FLinearColor::Red, FLinearColor::Yellow, FLinearColor::Green };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        Lights[Index]->SetupAttachment(RootComponent);
        Lights[Index]->SetRelativeLocation(FVector(-30.f, 700.f, 450.f - Index * 55.f));
        Lights[Index]->SetLightColor(Colors[Index]);
        Lights[Index]->SetIntensity(3000.f);
        Lights[Index]->SetAttenuationRadius(150.f);
    }
}

void ATrafficLightGimmick::BeginPlay()
{
    Super::BeginPlay();
    RefreshSignal();
}

void ATrafficLightGimmick::OnActivationChanged()
{
    bHasPreviousLocation = false;
    StoppedElapsed = 0.f;
    bReportedStop = false;
    RefreshSignal();
}

void ATrafficLightGimmick::RefreshSignal()
{
    RedLight->SetVisibility(bActive && Signal == ETrafficSignal::Red);
    YellowLight->SetVisibility(bActive && Signal == ETrafficSignal::Yellow);
    GreenLight->SetVisibility(bActive && Signal == ETrafficSignal::Green);
}

void ATrafficLightGimmick::SetSignal(ETrafficSignal NewSignal)
{
    if (!HasAuthority()) return;
    Signal = NewSignal;
    PhaseElapsed = 0.f;
    StoppedElapsed = 0.f;
    bReportedStop = false;
    RefreshSignal();
    ForceNetUpdate();
}

bool ATrafficLightGimmick::CrossesStopLine(const FVector& PreviousWorldLocation, const FVector& WorldLocation) const
{
    const FTransform Transform = StopZone->GetComponentTransform();
    const FVector Previous = Transform.InverseTransformPosition(PreviousWorldLocation);
    const FVector Current = Transform.InverseTransformPosition(WorldLocation);
    if (Previous.X >= 0.f || Current.X < 0.f) return false;
    const float Alpha = -Previous.X / (Current.X - Previous.X);
    const FVector Crossing = FMath::Lerp(Previous, Current, Alpha);
    const FVector Extent = StopZone->GetUnscaledBoxExtent();
    return FMath::Abs(Crossing.Y) <= Extent.Y && FMath::Abs(Crossing.Z) <= Extent.Z;
}

void ATrafficLightGimmick::UpdateGimmick(float DeltaSeconds)
{
    // Judge movement against the signal displayed during this interval, before advancing the cycle.
    ACoopVehicle* Vehicle = GetTargetVehicle();
    if (IsTargetVehicle(Vehicle))
    {
        const FVector Location = Vehicle->GetActorLocation();
        if (ObservedVehicle.Get() != Vehicle)
        {
            ObservedVehicle = Vehicle;
            bHasPreviousLocation = false;
            StoppedElapsed = 0.f;
            bReportedStop = false;
        }
        const bool bMustStop = Signal == ETrafficSignal::Red || (bTreatYellowAsRed && Signal == ETrafficSignal::Yellow);
        if (bHasPreviousLocation && bMustStop && CrossesStopLine(PreviousLocation, Location)) ApplyEffect(Vehicle);

        const FVector Local = StopZone->GetComponentTransform().InverseTransformPosition(Location);
        const FVector Extent = StopZone->GetUnscaledBoxExtent();
        const bool bInApproach = Local.X < 0.f && Local.X >= -Extent.X
            && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
        if (bMustStop && bInApproach && Vehicle->GetVelocity().Size() <= StoppedSpeedTolerance)
        {
            StoppedElapsed += DeltaSeconds;
            if (!bReportedStop && StoppedElapsed >= RequiredStopTime)
            {
                bReportedStop = true;
                OnVehicleStopped(Vehicle);
            }
        }
        else StoppedElapsed = 0.f;
        PreviousLocation = Location;
        bHasPreviousLocation = true;
    }
    else bHasPreviousLocation = false;

    if (bAutomaticCycle)
    {
        PhaseElapsed += DeltaSeconds;
        const float Duration = Signal == ETrafficSignal::Red ? RedDuration
            : Signal == ETrafficSignal::Green ? GreenDuration : YellowDuration;
        if (PhaseElapsed >= FMath::Max(0.1f, Duration))
        {
            SetSignal(Signal == ETrafficSignal::Red ? ETrafficSignal::Green
                : Signal == ETrafficSignal::Green ? ETrafficSignal::Yellow : ETrafficSignal::Red);
        }
    }
}

void ATrafficLightGimmick::ApplyEffect(ACoopVehicle* Vehicle)
{
    Super::ApplyEffect(Vehicle);
    UE_LOG(LogTemp, Log, TEXT("[Gimmick] %s ran the signal at %s"), *GetNameSafe(Vehicle), *GetName());
    // Penalty examples only. Connect these to the future cargo/economy system:
    // CargoInventory->RemoveCargo(CargoPenalty);
    // Wallet->SpendCurrency(CurrencyPenalty);
}

void ATrafficLightGimmick::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATrafficLightGimmick, Signal);
}
