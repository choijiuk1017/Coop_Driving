#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameMode/LobbyGameMode.h"
#include "Player/PlayerCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLobbyGroupSpawnTest, "CoopDriving.Lobby.GroupSpawn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLobbyGroupSpawnTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ALobbyGameMode* Mode = World->SpawnActor<ALobbyGameMode>();
    Mode->DefaultPawnClass = APlayerCharacter::StaticClass();
    APlayerStart* Center = World->SpawnActor<APlayerStart>(FVector(1000, 1000, 100), FRotator(0, 35, 0));
    Center->Tags.Add(TEXT("LobbySpawn"));
    // A remote start must never be used as fallback.
    World->SpawnActor<APlayerStart>(FVector(100000, 100000, 100), FRotator::ZeroRotator);
    APlayerController* First = World->SpawnActor<APlayerController>();
    Mode->RestartPlayer(First);
    TestNull(TEXT("No floor: wait instead of spawning in the air or far away"), First->GetPawn());

    AActor* Floor = World->SpawnActor<AActor>();
    UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(FloorBox);
    FloorBox->SetBoxExtent(FVector(2000, 2000, 50));
    FloorBox->SetCollisionProfileName(TEXT("BlockAll"));
    FloorBox->RegisterComponent();
    Floor->SetActorLocation(FVector(1000, 1000, -50));

    TArray<APlayerController*> Players = { First };
    for (int32 Index = 1; Index < 4; ++Index) Players.Add(World->SpawnActor<APlayerController>());
    for (APlayerController* Player : Players) Mode->RestartPlayer(Player);
    for (int32 Index = 0; Index < Players.Num(); ++Index)
    {
        ACharacter* Pawn = Cast<ACharacter>(Players[Index]->GetPawn());
        if (!TestNotNull(TEXT("All four players spawn once safe floor exists"), Pawn)) return false;
        TestTrue(TEXT("Players remain within the group radius"), FVector::Dist2D(Pawn->GetActorLocation(), Center->GetActorLocation()) <= 400.f);
        for (int32 OtherIndex = 0; OtherIndex < Index; ++OtherIndex)
        {
            const ACharacter* Other = CastChecked<ACharacter>(Players[OtherIndex]->GetPawn());
            TestTrue(TEXT("Every pair has capsule clearance"), FVector::Dist2D(Pawn->GetActorLocation(), Other->GetActorLocation())
                >= Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius() + Other->GetCapsuleComponent()->GetScaledCapsuleRadius() + 19.f);
        }
    }

    // Cover the entire local group. A fifth attempt must not use the distant start.
    AActor* Blocker = World->SpawnActor<AActor>();
    UBoxComponent* BlockerBox = NewObject<UBoxComponent>(Blocker);
    Blocker->SetRootComponent(BlockerBox);
    BlockerBox->SetBoxExtent(FVector(500, 500, 500));
    BlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
    BlockerBox->RegisterComponent();
    Blocker->SetActorLocation(FVector(1000, 1000, 100));
    APlayerController* Waiting = World->SpawnActor<APlayerController>();
    Mode->RestartPlayer(Waiting);
    TestNull(TEXT("Fully blocked group never spawns overlapped or at remote start"), Waiting->GetPawn());
    return true;
}

#endif
