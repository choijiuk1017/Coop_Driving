#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "GameMode/DrivingGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Gimmick/DeerGimmick.h"
#include "Gimmick/PoliceGimmick.h"
#include "Gimmick/TrafficLightGimmick.h"
#include "Vehicle/CargoSpeedBump.h"
#include "Vehicle/CoopVehicle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrafficStopLineTest, "CoopDriving.Gimmick.TrafficStopLine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficStopLineTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ATrafficLightGimmick* Light = World->SpawnActor<ATrafficLightGimmick>();
    if (!TestNotNull(TEXT("Traffic light"), Light)) return false;
    Light->SetActorTransform(FTransform(FRotator(0.f, 65.f, 0.f), FVector(1000.f, 2000.f, 300.f), FVector(2.f, 0.5f, 1.f)));
    const FTransform Frame = Light->StopZone->GetComponentTransform();
    auto Crosses = [Light, Frame](FVector From, FVector To)
    {
        return Light->CrossesStopLine(Frame.TransformPosition(From), Frame.TransformPosition(To));
    };
    TestTrue(TEXT("Fast crossing through entire rotated/scaled zone detected"), Crosses(FVector(-2000, 0, 0), FVector(2000, 0, 0)));
    TestFalse(TEXT("Reverse direction ignored"), Crosses(FVector(100, 0, 0), FVector(-100, 0, 0)));
    TestFalse(TEXT("Adjacent road ignored"), Crosses(FVector(-100, 601, 0), FVector(100, 601, 0)));
    TestFalse(TEXT("Overpass ignored"), Crosses(FVector(-100, 0, 301), FVector(100, 0, 301)));
    TestFalse(TEXT("Stopping before line is legal"), Crosses(FVector(-100, 0, 0), FVector(-1, 0, 0)));
    TestFalse(TEXT("Remaining beyond the line is not another crossing"), Crosses(FVector(1, 0, 0), FVector(100, 0, 0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrafficSignalRulesTest, "CoopDriving.Gimmick.TrafficSignalRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficSignalRulesTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ACoopVehicle* Vehicle = World->SpawnActor<ACoopVehicle>();
    ATrafficLightGimmick* Light = World->SpawnActor<ATrafficLightGimmick>();
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) || !TestNotNull(TEXT("Light"), Light)) return false;
    Light->SetTargetVehicle(Vehicle);
    Light->bAutomaticCycle = false;
    int32 Violations = 0;
    Light->OnEffectApplied.AddLambda([&Violations](ACoopVehicle*) { ++Violations; });
    auto DriveAcross = [Vehicle, Light]()
    {
        Vehicle->SetActorLocation(FVector(-1000.f, 0.f, 0.f));
        Light->Tick(0.01f);
        Vehicle->SetActorLocation(FVector(1000.f, 0.f, 0.f));
        Light->Tick(0.01f);
        Light->Tick(0.01f);
    };
    Light->SetSignal(ETrafficSignal::Red);
    DriveAcross();
    TestEqual(TEXT("Red crossing reports once, staying past line does not repeat"), Violations, 1);
    Light->SetSignal(ETrafficSignal::Green);
    DriveAcross();
    TestEqual(TEXT("Green departure allowed"), Violations, 1);
    Light->SetSignal(ETrafficSignal::Yellow);
    DriveAcross();
    TestEqual(TEXT("Yellow is legal by default"), Violations, 1);
    Light->bTreatYellowAsRed = true;
    DriveAcross();
    TestEqual(TEXT("Optional yellow violation"), Violations, 2);
    Light->SetGimmickActive(false);
    DriveAcross();
    TestEqual(TEXT("Inactive light does not judge"), Violations, 2);
    Vehicle->SetActorLocation(FVector(100.f, 0.f, 0.f));
    Light->SetGimmickActive(true);
    Light->Tick(0.01f);
    TestEqual(TEXT("Reactivation never compares against a stale position"), Violations, 2);

    Light->bAutomaticCycle = true;
    Light->SetSignal(ETrafficSignal::Red);
    Light->Tick(Light->RedDuration);
    TestTrue(TEXT("Red becomes green"), Light->GetSignal() == ETrafficSignal::Green);
    Light->Tick(Light->GreenDuration);
    TestTrue(TEXT("Green becomes yellow"), Light->GetSignal() == ETrafficSignal::Yellow);
    Light->Tick(Light->YellowDuration);
    TestTrue(TEXT("Yellow becomes red"), Light->GetSignal() == ETrafficSignal::Red);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeerSingleCollisionTest, "CoopDriving.Gimmick.DeerSingleCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeerSingleCollisionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
    Context.OwningGameInstance = Instance;
    World->SetGameInstance(Instance);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    ACoopVehicle* Vehicle = World->SpawnActor<ACoopVehicle>();
    ACoopVehicle* OtherVehicle = World->SpawnActor<ACoopVehicle>();
    ADeerGimmick* Deer = World->SpawnActor<ADeerGimmick>();
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) || !TestNotNull(TEXT("Other vehicle"), OtherVehicle)
        || !TestNotNull(TEXT("Deer"), Deer)) return false;
    Deer->SetTargetVehicle(Vehicle);
    // AActor::ProcessEvent rejects even native dynamic callbacks before world actor initialization.
    World->InitializeActorsForPlay(FURL());
    int32 Impacts = 0;
    Deer->OnEffectApplied.AddLambda([&Impacts](ACoopVehicle*) { ++Impacts; });
    const FHitResult Hit;
    Deer->Body->OnComponentHit.Broadcast(Deer->Body, OtherVehicle, OtherVehicle->GetMesh(), FVector::ZeroVector, Hit);
    TestEqual(TEXT("Unrelated vehicle does not consume deer"), Impacts, 0);
    Deer->Body->OnComponentHit.Broadcast(Deer->Body, Vehicle, Vehicle->GetMesh(), FVector::ZeroVector, Hit);
    Deer->Body->OnComponentHit.Broadcast(Deer->Body, Vehicle, Vehicle->GetMesh(), FVector::ZeroVector, Hit);
    TestEqual(TEXT("Multiple contact callbacks consume deer only once"), Impacts, 1);
    TestFalse(TEXT("Consumed deer stops crossing"), Deer->IsGimmickActive());
    TestTrue(TEXT("Destruction deferred until after contact resolution"), Deer->GetLifeSpan() > 0.f);
    TestTrue(TEXT("Deer keeps physical collision through the final impact"), Deer->Body->IsCollisionEnabled());

    ACargoSpeedBump* Bump = World->SpawnActor<ACargoSpeedBump>();
    if (!TestNotNull(TEXT("Existing speed bump"), Bump)) return false;
    TestTrue(TEXT("Speed bump participates in common hierarchy"), Bump->IsA<AGimmick>());
    Bump->SetGimmickActive(false);
    TestFalse(TEXT("Inactive bump has no physical collision"), Bump->Surface->IsCollisionEnabled());
    Bump->SetGimmickActive(true);
    TestTrue(TEXT("Active bump restores existing physical collision"), Bump->Surface->IsCollisionEnabled());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPolicePursuitTest, "CoopDriving.Gimmick.PolicePursuit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPolicePursuitTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ACoopVehicle* Vehicle = World->SpawnActor<ACoopVehicle>();
    APoliceGimmick* Police = World->SpawnActor<APoliceGimmick>();
    if (!TestNotNull(TEXT("Vehicle"), Vehicle) || !TestNotNull(TEXT("Police"), Police)) return false;
    Vehicle->SetActorLocation(FVector(10000.f, 0.f, 0.f));
    Police->SetTargetVehicle(Vehicle);
    Police->bGameOverOnCatch = false;
    Police->Tick(0.01f);
    TestTrue(TEXT("Pursuit starts behind target"), Police->GetActorLocation().X < Vehicle->GetActorLocation().X);
    int32 Catches = 0;
    Police->OnEffectApplied.AddLambda([&Catches](ACoopVehicle*) { ++Catches; });
    Police->SetActorLocation(Vehicle->GetActorLocation() - FVector(100.f, 0.f, 0.f));
    Police->Tick(0.01f);
    Police->Tick(0.01f);
    TestEqual(TEXT("Catch is applied exactly once"), Catches, 1);
    TestTrue(TEXT("Caught result retained for replication"), Police->bCaughtVehicle);
    TestFalse(TEXT("Catch ends pursuit"), Police->IsGimmickActive());

    Police->SetGimmickActive(true);
    Police->PursuitSpeed = 1.f;
    Police->PursuitDuration = 0.1f;
    Police->Tick(0.2f);
    TestFalse(TEXT("Surviving time limit ends pursuit"), Police->IsGimmickActive());
    TestFalse(TEXT("Timeout is an escape, not a catch"), Police->bCaughtVehicle);
    TestEqual(TEXT("Escape has no penalty"), Catches, 1);

    Vehicle->DisableDriving();
    Vehicle->SetBrake(0.f);
    Vehicle->SetThrottle(1.f);
    Vehicle->ChangeGear(-1);
    TestTrue(TEXT("Game over blocks further driving"), Vehicle->IsDrivingDisabled());
    TestEqual(TEXT("Later input cannot release game-over brake"), Vehicle->GetBrakePressure(), 1.f);
    TestEqual(TEXT("Later input cannot shift gear"), Vehicle->GetSelectedGear(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoliceRoundFailureTest, "CoopDriving.Gimmick.PoliceRoundFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceRoundFailureTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
    Context.OwningGameInstance = Instance;
    World->SetGameInstance(Instance);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };
    World->GetWorldSettings()->DefaultGameMode = ADrivingGameMode::StaticClass();
    if (!TestTrue(TEXT("Create driving game mode"), World->SetGameMode(FURL()))) return false;
    ACoopVehicle* Vehicle = World->SpawnActor<ACoopVehicle>();
    if (!TestNotNull(TEXT("Shared vehicle"), Vehicle)) return false;
    Vehicle->bSpawnCargo = false;
    Vehicle->bSpawnTestSpeedBump = false;
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();
    ADrivingGameMode* Mode = World->GetAuthGameMode<ADrivingGameMode>();
    if (!TestNotNull(TEXT("Driving mode"), Mode)) return false;
    TestEqual(TEXT("Mode adopts placed vehicle"), Mode->GetSharedVehicle(), Vehicle);
    TestFalse(TEXT("Null vehicle cannot fail round"), Mode->TryFailDriving(nullptr, nullptr));

    APoliceGimmick* Police = World->SpawnActor<APoliceGimmick>(FVector(1000.f, 0.f, 0.f), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Police"), Police)) return false;
    Police->bStartBehindVehicle = false;
    Police->SetActorLocation(Vehicle->GetActorLocation() - FVector(100.f, 0.f, 0.f));
    // No explicit target: use DrivingGameMode's shared vehicle, just as in a real driving map.
    Police->Tick(0.01f);
    TestTrue(TEXT("Police catches shared vehicle"), Police->bCaughtVehicle);
    TestTrue(TEXT("Default catch produces actual round failure"), Vehicle->IsDrivingDisabled());
    TestFalse(TEXT("Already failed round cannot fail twice"), Mode->TryFailDriving(Vehicle, Police));
    TestFalse(TEXT("Failed vehicle cannot complete destination"), Mode->TryFinishDriving(Vehicle));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlacedGimmicksTest, "CoopDriving.Gimmick.PlacedActorsWorkWithoutWiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlacedGimmicksTest::RunTest(const FString& Parameters)
{
    // Exercise level placement before BeginPlay, with no TargetVehicle assignments or BP calls.
    for (bool bUseDrivingMode : { true, false })
    {
        AddInfo(bUseDrivingMode ? TEXT("Driving map") : TEXT("Standalone vehicle test map"));
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
        Context.OwningGameInstance = Instance;
        World->SetGameInstance(Instance);
        ON_SCOPE_EXIT
        {
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        };
        if (bUseDrivingMode)
        {
            World->GetWorldSettings()->DefaultGameMode = ADrivingGameMode::StaticClass();
            if (!TestTrue(TEXT("Create map game mode"), World->SetGameMode(FURL()))) return false;
        }
        ACoopVehicle* Vehicle = World->SpawnActor<ACoopVehicle>(FVector(0, 0, 100), FRotator::ZeroRotator);
        ADeerGimmick* Deer = World->SpawnActor<ADeerGimmick>(FVector(10000, 0, 100), FRotator::ZeroRotator);
        APoliceGimmick* Police = World->SpawnActor<APoliceGimmick>(FVector(20000, 0, 100), FRotator::ZeroRotator);
        ATrafficLightGimmick* Light = World->SpawnActor<ATrafficLightGimmick>(FVector(30000, 0, 100), FRotator::ZeroRotator);
        ACargoSpeedBump* Bump = World->SpawnActor<ACargoSpeedBump>(FVector(40000, 0, 0), FRotator::ZeroRotator);
        ADeerGimmick* FastPassDeer = World->SpawnActor<ADeerGimmick>(FVector(50000, 0, 100), FRotator::ZeroRotator);
        if (!TestNotNull(TEXT("Placed vehicle"), Vehicle) || !TestNotNull(TEXT("Placed deer"), Deer)
            || !TestNotNull(TEXT("Placed police"), Police) || !TestNotNull(TEXT("Placed traffic light"), Light)
            || !TestNotNull(TEXT("Placed speed bump"), Bump) || !TestNotNull(TEXT("Copied deer"), FastPassDeer)) return false;
        Vehicle->bSpawnCargo = false;
        Vehicle->bSpawnTestSpeedBump = false;
        World->InitializeActorsForPlay(FURL());
        if (bUseDrivingMode) World->BeginPlay();
        else World->GetWorldSettings()->NotifyBeginPlay();

        TestEqual(TEXT("Automatic deer target without actor reference"), Deer->GetTargetVehicle(), Vehicle);
        TestEqual(TEXT("Automatic police target without actor reference"), Police->GetTargetVehicle(), Vehicle);
        TestEqual(TEXT("Automatic signal target without actor reference"), Light->GetTargetVehicle(), Vehicle);
        const FVector DeerPlacement = Deer->GetActorLocation();
        const FVector PolicePlacement = Police->GetActorLocation();
        Deer->Tick(60.f);
        Police->Tick(60.f);
        TestTrue(TEXT("Distant deer waits even after crossing lifetime"), Deer->IsWaitingForVehicle());
        TestTrue(TEXT("Distant police waits even after pursuit duration"), Police->IsWaitingForVehicle());
        TestTrue(TEXT("Waiting deer remains at designer placement"), Deer->GetActorLocation().Equals(DeerPlacement));
        TestTrue(TEXT("Waiting police remains at designer placement"), Police->GetActorLocation().Equals(PolicePlacement));
        TestFalse(TEXT("Waiting deer has no gravity drift"), Deer->Body->IsSimulatingPhysics());
        TestFalse(TEXT("Waiting deer does not collide"), Deer->Body->IsCollisionEnabled());
        TestFalse(TEXT("Waiting police does not collide"), Police->Body->IsCollisionEnabled());

        Vehicle->SetActorLocation(DeerPlacement);
        Deer->Tick(0.01f);
        TestFalse(TEXT("Deer automatically starts when vehicle approaches"), Deer->IsWaitingForVehicle());
        TestTrue(TEXT("Crossing deer enables physical body"), Deer->Body->IsSimulatingPhysics());
        TestTrue(TEXT("Crossing deer is visible"), Deer->Body->IsVisible());
        TestTrue(TEXT("Crossing deer starts moving"), Deer->Body->GetPhysicsLinearVelocity().Size2D() > 0.f);

        Vehicle->SetActorLocation(PolicePlacement + FVector(0, 0, 2000));
        Police->Tick(0.01f);
        TestTrue(TEXT("Overpass does not start police"), Police->IsWaitingForVehicle());
        Vehicle->SetActorLocation(PolicePlacement - FVector(1000, 0, 0));
        Police->Tick(0.01f);
        TestFalse(TEXT("Police automatically starts at placed encounter zone"), Police->IsWaitingForVehicle());
        TestTrue(TEXT("Police appears behind approaching vehicle"), Police->GetActorLocation().X < Vehicle->GetActorLocation().X);
        TestFalse(TEXT("Pursuit did not expire while waiting"), Police->bCaughtVehicle);
        TestTrue(TEXT("Pursuit remains active after approach"), Police->IsGimmickActive());

        int32 Violations = 0;
        Light->OnEffectApplied.AddLambda([&Violations](ACoopVehicle*) { ++Violations; });
        Vehicle->SetActorLocation(FVector(29000, 0, 100));
        Light->Tick(0.01f);
        Vehicle->SetActorLocation(FVector(31000, 0, 100));
        Light->Tick(0.01f);
        TestEqual(TEXT("Placed light judges red crossing with default settings"), Violations, 1);
        TestTrue(TEXT("Placed speed bump immediately blocks"), Bump->Surface->IsCollisionEnabled());

        Vehicle->SetActorLocation(FVector(45000, 0, 100));
        FastPassDeer->Tick(0.01f);
        Vehicle->SetActorLocation(FVector(55000, 0, 100));
        FastPassDeer->Tick(0.01f);
        TestFalse(TEXT("High-speed full-zone crossing still activates a copied hazard"), FastPassDeer->IsWaitingForVehicle());

        // Default game-over behaviour is also functional in a standalone test map.
        Police->SetActorLocation(Vehicle->GetActorLocation() - FVector(100, 0, 0));
        Police->Tick(0.01f);
        TestTrue(TEXT("Police catches vehicle without extra event wiring"), Police->bCaughtVehicle);
        TestTrue(TEXT("Default capture blocks driving"), Vehicle->IsDrivingDisabled());
    }
    return true;
}

#endif
