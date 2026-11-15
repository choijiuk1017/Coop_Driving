#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Vehicle/DrivingInputRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoopBrakeTapTest, "CoopDriving.Input.BrakeRequiresRepeatedTaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoopBrakeTapTest::RunTest(const FString& Parameters)
{
    FBrakeTapState Brake;
    Brake.Tap(0.0, 0.6f, 0.35f);
    TestEqual(TEXT("A single press does not brake"), Brake.Pressure, 0.f);
    Brake.Tap(0.2, 0.6f, 0.35f);
    TestEqual(TEXT("Second tap builds pressure"), Brake.Pressure, 0.35f);
    Brake.Tap(0.21, 0.6f, 0.35f);
    TestEqual(TEXT("Duplicate packet cannot add pressure"), Brake.Pressure, 0.35f);
    Brake.Tap(0.4, 0.6f, 0.35f);
    Brake.Tap(0.6, 0.6f, 0.35f);
    TestEqual(TEXT("Pressure is capped"), Brake.Pressure, 1.f);
    Brake.Decay(2.f, 0.6f);
    TestEqual(TEXT("Holding/stopping taps releases brake"), Brake.Pressure, 0.f);
    Brake.Tap(3.0, 0.6f, 0.35f);
    TestEqual(TEXT("A late tap only primes a new series"), Brake.Pressure, 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoopGearPatternTest, "CoopDriving.Input.GearPatterns",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoopGearPatternTest::RunTest(const FString& Parameters)
{
    const TArray<EGearArrow> Up = { EGearArrow::Up, EGearArrow::Right, EGearArrow::Down, EGearArrow::Up };
    const TArray<EGearArrow> Down = { EGearArrow::Down, EGearArrow::Left, EGearArrow::Up, EGearArrow::Down };
    FGearPatternState State;
    for (int32 Index = 0; Index < Up.Num(); ++Index)
        TestEqual(TEXT("Only the complete up pattern shifts"), State.Push(Up[Index], Index * 0.2, 2.f, Up, Down), Index == 3 ? 1 : 0);
    TestTrue(TEXT("Completion clears progress"), State.Entered.IsEmpty());
    for (int32 Index = 0; Index < Down.Num(); ++Index)
        TestEqual(TEXT("Complete down pattern shifts down"), State.Push(Down[Index], 1.0 + Index * 0.2, 2.f, Up, Down), Index == 3 ? -1 : 0);
    State.Push(EGearArrow::Up, 2.0, 2.f, Up, Down);
    TestEqual(TEXT("Wrong arrow rejects pattern"), State.Push(EGearArrow::Left, 2.1, 2.f, Up, Down), 2);
    TestTrue(TEXT("Wrong input clears progress"), State.Entered.IsEmpty());
    State.Push(EGearArrow::Up, 3.0, 2.f, Up, Down);
    TestFalse(TEXT("A timely next key remains valid"), State.Expire(4.9, 2.f));
    TestTrue(TEXT("Idle pattern expires"), State.Expire(5.1, 2.f));
    TestEqual(TEXT("Suffix after timeout cannot shift"), State.Push(EGearArrow::Right, 5.2, 2.f, Up, Down), 2);
    TestEqual(TEXT("Empty patterns cannot shift"), State.Push(EGearArrow::Up, 6.0, 2.f, {}, {}), 2);
    return true;
}

#endif
