// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameMode/DrivingGameMode.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Online/LANSessionSubsystem.h"
#include "Player/PlayerRoleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ALobbyGameMode::ALobbyGameMode()
{
    // Keep client connections during level travel.
    bUseSeamlessTravel = true;
    InGameGameMode = TSoftClassPtr<ADrivingGameMode>(FSoftObjectPath(
        TEXT("/Game/Blueprint/BP_DrivingGameMode.BP_DrivingGameMode_C")));
}

int32 ALobbyGameMode::GetRequiredPlayers() const
{
    return RequiredPlayers;
}

bool ALobbyGameMode::StartGameLevel()
{
    if (!HasAuthority() || bTravelStarted)
    {
        return false;
    }

    if (GetNumPlayers() != RequiredPlayers)
    {
        return false;
    }

    const FString MapPath = DrivingMap.IsNull() ? NextLevelPath : DrivingMap.ToSoftObjectPath().GetLongPackageName();
    if (!GetWorld() || MapPath.IsEmpty() || !FPackageName::DoesPackageExist(MapPath))
    {
        return false;
    }

    UClass* DestinationGameMode = InGameGameMode.LoadSynchronous();
    if (!DestinationGameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] Cannot travel: InGameGameMode is not configured or could not be loaded."));
        return false;
    }

    bTravelStarted = true;

    if (ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>())
    {
        LAN->CloseLobby();
    }

    // Carry the actual lobby and its BP defaults through the entire round.
    const FString LobbyPath = FPackageName::GetLongPackagePath(GetWorld()->GetOutermost()->GetName())
        + TEXT("/") + UGameplayStatics::GetCurrentLevelName(this, true);
    const FString TravelURL = MapPath + TEXT("?listen?game=") + DestinationGameMode->GetPathName()
        + TEXT("?ReturnLobbyMap=") + LobbyPath
        + TEXT("?ReturnLobbyGameMode=") + GetClass()->GetPathName();

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ServerTravel: %s"), *TravelURL);

    const bool bSuccess = GetWorld()->ServerTravel(TravelURL);

    if (!bSuccess)
    {
        bTravelStarted = false;
        if (ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>())
        {
            LAN->ReopenLobby();
        }
    }

    return bSuccess;
}

void ALobbyGameMode::SetDrivingMap(TSoftObjectPtr<UWorld> Map)
{
    if (HasAuthority() && !bTravelStarted) DrivingMap = Map;
}

void ALobbyGameMode::ResetLobbyPlayer(APlayerController* Player)
{
    APlayerState* State = Player ? Player->GetPlayerState<APlayerState>() : nullptr;
    if (UPlayerRoleComponent* Roles = State ? State->FindComponentByClass<UPlayerRoleComponent>() : nullptr)
        Roles->AssignRole(EDrivingRole::None);
}

void ALobbyGameMode::StartPlay()
{
    Super::StartPlay();
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        ResetLobbyPlayer(It->Get());
    if (ULANSessionSubsystem* LAN = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>())
        LAN->ReopenLobby();
}

void ALobbyGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    ResetLobbyPlayer(NewPlayer);
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

AActor* ALobbyGameMode::GetSpawnCenter()
{
    if (CachedSpawnCenter.IsValid()) return CachedSpawnCenter.Get();
    TArray<APlayerStart*> Starts;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        Starts.Add(*It);
    Starts.Sort([](const APlayerStart& A, const APlayerStart& B) { return A.GetName() < B.GetName(); });
    for (APlayerStart* Start : Starts)
    {
        if (Start->ActorHasTag(SpawnCenterTag) || Start->PlayerStartTag == SpawnCenterTag)
        {
            CachedSpawnCenter = Start;
            return Start;
        }
    }
    if (!Starts.IsEmpty()) CachedSpawnCenter = Starts[0];
    return CachedSpawnCenter.Get();
}

bool ALobbyGameMode::FindLobbySpawn(AController* Player, FTransform& Transform)
{
    const AActor* Center = GetSpawnCenter();
    const UClass* PawnClass = GetDefaultPawnClassForController(Player);
    const ACharacter* Defaults = PawnClass ? Cast<ACharacter>(PawnClass->GetDefaultObject()) : nullptr;
    if (!Center || !Defaults) return false;
    const UCapsuleComponent* Capsule = Defaults->GetCapsuleComponent();
    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float Clearance = FMath::Max(1.f, SpawnClearance);
    const float Spacing = FMath::Max(SpawnSpacing, 2.f * Radius + Clearance + 2.f);
    const float MaxRadius = FMath::Max(1.f, SpawnMaxRadius);
    const FRotator Rotation(0.f, Center->GetActorRotation().Yaw, 0.f);
    const FVector Origin = Center->GetActorLocation();

    // Prefer a compact 2x2 group. Additional candidates remain inside the same bounded area.
    TArray<FVector> Offsets = {
        FVector(-Spacing / 2, -Spacing / 2, 0), FVector(-Spacing / 2, Spacing / 2, 0),
        FVector(Spacing / 2, -Spacing / 2, 0), FVector(Spacing / 2, Spacing / 2, 0)
    };
    // Bound work even when the editor settings specify an unusually large radius.
    const int32 Steps = FMath::Min(8, FMath::FloorToInt(MaxRadius / Spacing));
    TArray<FVector> Alternatives;
    for (int32 X = -Steps; X <= Steps; ++X)
        for (int32 Y = -Steps; Y <= Steps; ++Y)
            Alternatives.Add(FVector(X * Spacing, Y * Spacing, 0));
    Alternatives.Sort([](const FVector& A, const FVector& B) { return A.SizeSquared() < B.SizeSquared(); });
    Offsets.Append(Alternatives);

    FCollisionQueryParams Query(SCENE_QUERY_STAT(LobbySpawn), false);
    for (const FVector& Offset : Offsets)
    {
        if (Offset.SizeSquared2D() > FMath::Square(MaxRadius)) continue;
        FVector Location = Origin + Rotation.RotateVector(Offset);
        FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByProfile(Floor, Location + FVector(0, 0, 200.f),
            Location - FVector(0, 0, 400.f), Capsule->GetCollisionProfileName(), Query)
            || Floor.ImpactNormal.Z < Defaults->GetCharacterMovement()->GetWalkableFloorZ()) continue;
        Location.Z = Floor.ImpactPoint.Z + HalfHeight + 2.f;
        if (FMath::Abs(Location.Z - Origin.Z) > 200.f) continue;
        // Inflate only horizontally so a valid floor is not treated as an obstruction.
        if (GetWorld()->OverlapBlockingTestByProfile(Location, FQuat::Identity, Capsule->GetCollisionProfileName(),
            FCollisionShape::MakeCapsule(Radius + Clearance / 2, HalfHeight), Query)) continue;

        bool bTooClose = false;
        for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
        {
            const UCapsuleComponent* Other = It->GetCapsuleComponent();
            if (FVector::Dist2D(Location, It->GetActorLocation()) < Radius + Other->GetScaledCapsuleRadius() + Clearance)
            {
                bTooClose = true;
                break;
            }
        }
        if (bTooClose) continue;
        Transform = FTransform(Rotation, Location);
        return true;
    }
    return false;
}

void ALobbyGameMode::RestartPlayer(AController* NewPlayer)
{
    if (!HasAuthority() || bTravelStarted || !IsValid(NewPlayer) || NewPlayer->IsPendingKillPending()
        || NewPlayer->GetPawn() || MustSpectate(Cast<APlayerController>(NewPlayer))) return;
    FTransform Transform;
    APawn* Pawn = nullptr;
    if (FindLobbySpawn(NewPlayer, Transform))
    {
        FActorSpawnParameters Params;
        Params.Instigator = GetInstigator();
        Params.ObjectFlags |= RF_Transient;
        // Do not allow engine adjustment to move this pawn outside the checked group.
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        Pawn = GetWorld()->SpawnActor<APawn>(GetDefaultPawnClassForController(NewPlayer), Transform, Params);
    }
    if (Pawn)
    {
        NewPlayer->SetPawn(Pawn);
        PendingSpawns.Remove(NewPlayer);
        FinishRestartPlayer(NewPlayer, Transform.Rotator());
        UE_LOG(LogTemp, Log, TEXT("[LobbySpawn] %s at %s"), *NewPlayer->GetName(), *Pawn->GetActorLocation().ToString());
    }
    else
    {
        // A blocked group waits for free space instead of spawning overlapped or far away.
        if (!PendingSpawns.Contains(NewPlayer))
            UE_LOG(LogTemp, Warning, TEXT("[LobbySpawn] Waiting for safe space near the lobby PlayerStart for %s"), *NewPlayer->GetName());
        PendingSpawns.AddUnique(NewPlayer);
        if (!GetWorldTimerManager().IsTimerActive(SpawnRetryTimer))
            GetWorldTimerManager().SetTimer(SpawnRetryTimer, this, &ThisClass::RetryPendingSpawns, 0.5f, true);
    }
}

void ALobbyGameMode::RetryPendingSpawns()
{
    const TArray<TWeakObjectPtr<AController>> Players = PendingSpawns;
    for (const TWeakObjectPtr<AController>& Player : Players)
    {
        if (!Player.IsValid() || Player->IsPendingKillPending() || Player->GetPawn()) PendingSpawns.Remove(Player);
        else RestartPlayer(Player.Get());
    }
    if (PendingSpawns.IsEmpty() || bTravelStarted) GetWorldTimerManager().ClearTimer(SpawnRetryTimer);
}
