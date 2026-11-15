#pragma once

#include "CoreMinimal.h"
#include "DrivingInputRules.generated.h"

UENUM(BlueprintType)
enum class EGearArrow : uint8
{
    Up,
    Down,
    Left,
    Right
};

// Server-owned state: first tap primes the brake, repeated taps build pressure.
struct FBrakeTapState
{
    float Pressure = 0.f;
    double LastTapTime = -1.0;

    void Tap(double Now, float Window, float Gain)
    {
        if (LastTapTime >= 0.0 && Now - LastTapTime < 0.06) return;
        if (LastTapTime >= 0.0 && Now - LastTapTime <= Window)
            Pressure = FMath::Clamp(Pressure + Gain, 0.f, 1.f);
        LastTapTime = Now;
    }

    void Decay(float DeltaSeconds, float Rate)
    {
        Pressure = FMath::Max(0.f, Pressure - FMath::Max(0.f, DeltaSeconds) * Rate);
    }
};

struct FGearPatternState
{
    TArray<EGearArrow> Entered;
    double LastInputTime = -1.0;

    bool Expire(double Now, float Timeout)
    {
        if (!Entered.IsEmpty() && Now - LastInputTime > Timeout)
        {
            Entered.Reset();
            return true;
        }
        return false;
    }

    static bool IsPrefix(const TArray<EGearArrow>& Input, const TArray<EGearArrow>& Pattern)
    {
        if (Pattern.IsEmpty() || Input.Num() > Pattern.Num()) return false;
        for (int32 Index = 0; Index < Input.Num(); ++Index)
            if (Input[Index] != Pattern[Index]) return false;
        return true;
    }

    // 1/-1: completed shift, 0: pending, 2: incorrect input.
    int32 Push(EGearArrow Arrow, double Now, float Timeout,
        const TArray<EGearArrow>& Up, const TArray<EGearArrow>& Down)
    {
        Expire(Now, Timeout);
        LastInputTime = Now;
        Entered.Add(Arrow);
        const bool bUp = IsPrefix(Entered, Up);
        const bool bDown = IsPrefix(Entered, Down);
        const int32 Result = bUp && Entered.Num() == Up.Num() ? 1
            : bDown && Entered.Num() == Down.Num() ? -1
            : !bUp && !bDown ? 2 : 0;
        if (Result != 0) Entered.Reset();
        return Result;
    }
};
