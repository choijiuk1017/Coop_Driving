#pragma once

#include "Player/PlayerRoleComponent.h"

// Complete the existing assignment without moving any already assigned player.
// Invalid/duplicate roles fail atomically instead of silently reshuffling clients.
inline bool CompleteDrivingRoleAssignment(const TArray<EDrivingRole>& Existing, TArray<EDrivingRole>& Result)
{
    Result.Reset();
    TArray<EDrivingRole> Remaining = {
        EDrivingRole::Briefing, EDrivingRole::Steering, EDrivingRole::Pedals, EDrivingRole::Gear
    };
    if (Existing.Num() != Remaining.Num()) return false;
    for (EDrivingRole Assigned : Existing)
    {
        if (Assigned != EDrivingRole::None && Remaining.RemoveSingle(Assigned) != 1) return false;
    }
    for (int32 Index = Remaining.Num() - 1; Index > 0; --Index)
        Remaining.Swap(Index, FMath::RandRange(0, Index));
    Result = Existing;
    for (EDrivingRole& Assigned : Result)
        if (Assigned == EDrivingRole::None) Assigned = Remaining.Pop();
    return true;
}
