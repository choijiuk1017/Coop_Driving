#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Player/DrivingRoleAssignment.h"
#include "Player/MainPlayerController.h"
#include "Vehicle/DrivingPlayerController.h"
#include "Online/LANPlayerController.h"
#include "Online/LANGameModeBase.h"
#include "GameMode/DrivingGameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrivingRoleIntegrationTest, "CoopDriving.Integration.PreserveAssignedClients",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrivingRoleIntegrationTest::RunTest(const FString& Parameters)
{
    const TArray<EDrivingRole> Existing = {
        EDrivingRole::Gear, EDrivingRole::Briefing, EDrivingRole::Pedals, EDrivingRole::Steering
    };
    TArray<EDrivingRole> Result;
    TestTrue(TEXT("An existing complete assignment is accepted"), CompleteDrivingRoleAssignment(Existing, Result));
    TestTrue(TEXT("Every client retains its original role"), Existing == Result);
    for (int32 Trial = 0; Trial < 100; ++Trial)
    {
        TArray<EDrivingRole> Partial = { EDrivingRole::None, EDrivingRole::Steering, EDrivingRole::None, EDrivingRole::None };
        TestTrue(TEXT("Only missing assignments are filled"), CompleteDrivingRoleAssignment(Partial, Result));
        TestTrue(TEXT("The assigned steering client never changes"), Result[1] == EDrivingRole::Steering);
        TestEqual(TEXT("Every role is unique"), TSet<EDrivingRole>(Result).Num(), 4);
        TestFalse(TEXT("All clients receive a role"), Result.Contains(EDrivingRole::None));
    }
    TestTrue(TEXT("Direct four-player test can assign from scratch"), CompleteDrivingRoleAssignment(
        { EDrivingRole::None, EDrivingRole::None, EDrivingRole::None, EDrivingRole::None }, Result));
    TestEqual(TEXT("New random assignments have four unique roles"), TSet<EDrivingRole>(Result).Num(), 4);
    TestFalse(TEXT("Wait for all four arrivals"), CompleteDrivingRoleAssignment({ EDrivingRole::Gear }, Result));
    TestFalse(TEXT("Duplicate assignments fail without reshuffling players"), CompleteDrivingRoleAssignment(
        { EDrivingRole::Gear, EDrivingRole::Gear, EDrivingRole::None, EDrivingRole::None }, Result));
    TestTrue(TEXT("Failed assignment produces no partial plan"), Result.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrivingLANCompatibilityTest, "CoopDriving.Integration.LANControllerCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrivingLANCompatibilityTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Walking retains shared voice/leave policy"), AMainPlayerController::StaticClass()->IsChildOf(ALANPlayerController::StaticClass()));
    TestTrue(TEXT("Driving retains shared voice/leave policy"), ADrivingPlayerController::StaticClass()->IsChildOf(ALANPlayerController::StaticClass()));
    TestTrue(TEXT("Driving retains LAN admission policy"), ADrivingGameMode::StaticClass()->IsChildOf(ALANGameModeBase::StaticClass()));
    return true;
}

#endif
