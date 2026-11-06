#include "GameMode/DrivingGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/CoopPlayerState.h"
#include "Player/PlayerRoleComponent.h"
void ADrivingGameMode::HandleStartingNewPlayer_Implementation(
    APlayerController* NewPlayer)
{
    if (!IsValid(NewPlayer))
    {
        return;
    }

    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    if (bRolesAssigned)
    {
        return;
    }

    APlayerState* PlayerState =
        NewPlayer->GetPlayerState<APlayerState>();

    if (!IsValid(PlayerState) || PlayerState->IsOnlyASpectator())
    {
        return;
    }

    ArrivedPlayers.RemoveAll(
        [](const TWeakObjectPtr<APlayerController>& Player)
    {
        return !Player.IsValid();
    });

    ArrivedPlayers.AddUnique(
        TWeakObjectPtr<APlayerController>(NewPlayer));

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[Role] Arrived: %s (%d/%d)"),
        *NewPlayer->GetName(),
        ArrivedPlayers.Num(),
        RequiredPlayers);

    TryAssignRoles();
}

void ADrivingGameMode::Logout(AController* Exiting)
{
    ArrivedPlayers.RemoveAll(
        [Exiting](const TWeakObjectPtr<APlayerController>& Player)
    {
        return !Player.IsValid() || Player.Get() == Exiting;
    });

    Super::Logout(Exiting);
}

void ADrivingGameMode::TryAssignRoles()
{
    if (!HasAuthority() || bRolesAssigned)
    {
        return;
    }

    ArrivedPlayers.RemoveAll(
        [](const TWeakObjectPtr<APlayerController>& Player)
    {
        return !Player.IsValid();
    });

    if (ArrivedPlayers.Num() != RequiredPlayers)
    {
        return;
    }

    TArray<UPlayerRoleComponent*> RoleComponents;

    for (const auto& Player : ArrivedPlayers)
    {
        APlayerController* Controller = Player.Get();

        ACoopPlayerState* PlayerState = Controller
            ? Controller->GetPlayerState<ACoopPlayerState>()
            : nullptr;

        UPlayerRoleComponent* Component = IsValid(PlayerState)
            ? PlayerState->FindComponentByClass<UPlayerRoleComponent>()
            : nullptr;

        if (!IsValid(Component))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[Role] Assignment aborted: missing PlayerState or RoleComponent."));
            return;
        }

        RoleComponents.Add(Component);
    }

    TArray<EDrivingRole> Roles = {
        EDrivingRole::Briefing,
        EDrivingRole::Steering,
        EDrivingRole::Pedals,
        EDrivingRole::Gear
    };

    for (int32 Index = Roles.Num() - 1; Index > 0; --Index)
    {
        const int32 RandomIndex = FMath::RandRange(0, Index);
        Roles.Swap(Index, RandomIndex);
    }

    bRolesAssigned = true;

    for (int32 Index = 0; Index < RoleComponents.Num(); ++Index)
    {
        RoleComponents[Index]->AssignRole(Roles[Index]);
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[Role] Random assignment completed for %d players."),
        RoleComponents.Num());
}