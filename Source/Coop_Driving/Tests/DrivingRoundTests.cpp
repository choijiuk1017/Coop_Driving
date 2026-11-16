#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Actor/DrivingDestinationZone.h"
#include "Player/CoopPlayerState.h"
#include "Player/PlayerRoleComponent.h"
#include "Player/DrivingRoleAssignment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrivingRoundResetTest, "CoopDriving.Round.ResetAndReassign",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrivingRoundResetTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ACoopPlayerState* Source = World->SpawnActor<ACoopPlayerState>();
    ACoopPlayerState* Target = World->SpawnActor<ACoopPlayerState>();
    if (!TestNotNull(TEXT("Source player state"), Source) || !TestNotNull(TEXT("Travel player state"), Target)) return false;
    UPlayerRoleComponent* Roles = Source->FindComponentByClass<UPlayerRoleComponent>();
    UPlayerRoleComponent* Copied = Target->FindComponentByClass<UPlayerRoleComponent>();
    if (!TestNotNull(TEXT("Role component"), Roles) || !TestNotNull(TEXT("Travel role component"), Copied)) return false;
    const TArray<EDrivingRole> AllRoles = {
        EDrivingRole::Briefing, EDrivingRole::Steering, EDrivingRole::Pedals, EDrivingRole::Gear
    };
    for (EDrivingRole Role : AllRoles)
    {
        Roles->AssignRole(Role);
        TestFalse(TEXT("Assigned role has disabilities"), Roles->GetAssignment().DisabilityIds.IsEmpty());
        Roles->AssignRole(EDrivingRole::None);
        TestTrue(TEXT("Round finish clears role"), Roles->GetRole() == EDrivingRole::None);
        TestTrue(TEXT("Round finish clears every disability"), Roles->GetAssignment().DisabilityIds.IsEmpty());
        Copied->AssignRole(EDrivingRole::Pedals);
        Source->CopyProperties(Target);
        TestTrue(TEXT("Seamless travel preserves cleared role"), Copied->GetRole() == EDrivingRole::None);
        TestTrue(TEXT("Seamless travel preserves cleared disabilities"), Copied->GetAssignment().DisabilityIds.IsEmpty());
    }
    TArray<EDrivingRole> NextRound;
    TestTrue(TEXT("Four cleared players can start another round"), CompleteDrivingRoleAssignment(
        { Roles->GetRole(), Copied->GetRole(), EDrivingRole::None, EDrivingRole::None }, NextRound));
    TestEqual(TEXT("Next round has four distinct roles"), TSet<EDrivingRole>(NextRound).Num(), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrivingDestinationBoundsTest, "CoopDriving.Round.DestinationBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrivingDestinationBoundsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ADrivingDestinationZone* Zone = World->SpawnActor<ADrivingDestinationZone>();
    if (!TestNotNull(TEXT("Destination zone"), Zone)) return false;
    Zone->SetActorTransform(FTransform(FRotator(0.f, 70.f, 0.f), FVector(1500.f, -900.f, 200.f), FVector(2.f, 0.5f, 1.f)));
    const FTransform Transform = Zone->GetActorTransform();
    TestTrue(TEXT("Vehicle origin inside rotated/scaled zone completes"), Zone->ContainsLocation(Transform.TransformPosition(FVector(390.f, 390.f, 290.f))));
    TestFalse(TEXT("Vehicle outside side does not complete"), Zone->ContainsLocation(Transform.TransformPosition(FVector(410.f, 0.f, 0.f))));
    TestFalse(TEXT("Vehicle above zone does not complete"), Zone->ContainsLocation(Transform.TransformPosition(FVector(0.f, 0.f, 310.f))));
    return true;
}

#endif
