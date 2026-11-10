#include "GameMode/DrivingGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/CoopPlayerState.h"
#include "Player/PlayerRoleComponent.h"
#include "Vehicle/CoopVehicle.h"
#include "Vehicle/DrivingPlayerController.h"
#include "Vehicle/DrivingHUD.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ADrivingGameMode::ADrivingGameMode()
{
    PlayerControllerClass = ADrivingPlayerController::StaticClass();
    static ConstructorHelpers::FClassFinder<ADrivingPlayerController> DriverBP(TEXT("/Game/Blueprint/BP_DrivingPlayerController"));
    if (DriverBP.Succeeded()) PlayerControllerClass = DriverBP.Class;
    PlayerStateClass = ACoopPlayerState::StaticClass();
    DefaultPawnClass = APawn::StaticClass();
    HUDClass = ADrivingHUD::StaticClass();
    static ConstructorHelpers::FClassFinder<ACoopVehicle> VehicleBP(TEXT("/Game/Blueprint/BP_TestVehicle"));
    SharedVehicleClass = VehicleBP.Class;
}

void ADrivingGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    // Old BP defaults still reference the walking controller. Upgrade only incompatible classes.
    if (!PlayerControllerClass || !PlayerControllerClass->IsChildOf(ADrivingPlayerController::StaticClass()))
        PlayerControllerClass = GetDefault<ADrivingGameMode>()->PlayerControllerClass;
    if (!PlayerStateClass || !PlayerStateClass->IsChildOf(ACoopPlayerState::StaticClass()))
        PlayerStateClass = ACoopPlayerState::StaticClass();
    DefaultPawnClass = APawn::StaticClass(); // Empty player pawns; the shared vehicle stays unpossessed.
    if (!HUDClass || !HUDClass->IsChildOf(ADrivingHUD::StaticClass())) HUDClass = ADrivingHUD::StaticClass();
    Super::InitGame(MapName, Options, ErrorMessage);
}

void ADrivingGameMode::EnsureSharedVehicle()
{
    if (IsValid(SharedVehicle)) return;
    for (TActorIterator<ACoopVehicle> It(GetWorld()); It; ++It)
    {
        SharedVehicle = *It;
        break;
    }
    if (!SharedVehicle && SharedVehicleClass)
    {
        FTransform SpawnTransform = FTransform::Identity;
        for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        {
            SpawnTransform = It->GetActorTransform();
            break;
        }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        SharedVehicle = GetWorld()->SpawnActor<ACoopVehicle>(SharedVehicleClass, SpawnTransform, Params);
    }
    if (!SharedVehicle) UE_LOG(LogTemp, Error, TEXT("[Driving] No shared vehicle. Set SharedVehicleClass in the game mode."));
}

void ADrivingGameMode::ConnectVehicle(APlayerController* Player)
{
    if (ADrivingPlayerController* Driver = Cast<ADrivingPlayerController>(Player))
    {
        EnsureSharedVehicle();
        Driver->SetSharedVehicle(SharedVehicle);
    }
}

void ADrivingGameMode::StartPlay()
{
    Super::StartPlay();
    EnsureSharedVehicle();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        ConnectVehicle(It->Get());
}

void ADrivingGameMode::HandleStartingNewPlayer_Implementation(
    APlayerController* NewPlayer)
{
    if (!IsValid(NewPlayer))
    {
        return;
    }

    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
    ConnectVehicle(NewPlayer);

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
    if (ADrivingPlayerController* Driver = Cast<ADrivingPlayerController>(Exiting)) Driver->ClearDrivingInput();
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
