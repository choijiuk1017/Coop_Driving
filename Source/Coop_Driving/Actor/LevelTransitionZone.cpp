// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/LevelTransitionZone.h"

#include "Player/PlayerCharacter.h"

#include "GameMode/LobbyGameMode.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
// Sets default values
ALevelTransitionZone::ALevelTransitionZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;


    bReplicates = true;
    SetReplicateMovement(false);

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));

    SetRootComponent(TriggerBox);

    TriggerBox->SetBoxExtent(FVector(250.f, 250.f, 150.f));

    TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);

    TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    TriggerBox->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void ALevelTransitionZone::BeginPlay()
{
	Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ALevelTransitionZone::OnBeginOverlap);

    TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ALevelTransitionZone::OnEndOverlap);

    GetWorldTimerManager().SetTimer(ZoneCheckTimer, this, &ALevelTransitionZone::UpdateZone, CheckInterval, true);

    UpdateZone();
	
}

void ALevelTransitionZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ZoneCheckTimer);

    PlayersInZone.Empty();

    Super::EndPlay(EndPlayReason);
}

void ALevelTransitionZone::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    UpdateZone();
}

void ALevelTransitionZone::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority()) return;

    UpdateZone();
}

void ALevelTransitionZone::UpdateZone()
{
    if (!HasAuthority() || bTransitionRequested) return;

    UWorld* World = GetWorld();

    if (!World || !TriggerBox) return;

    ALobbyGameMode* GameMode = Cast<ALobbyGameMode>(World->GetAuthGameMode());

    if (!GameMode) return;

    TArray<AActor*> OverlappingActors;

    TriggerBox->GetOverlappingActors(OverlappingActors, APlayerCharacter::StaticClass());

    TSet<TWeakObjectPtr<APlayerController>> CurrentPlayers;

    for (AActor* Actor : OverlappingActors)
    {
        APlayerCharacter* Character = Cast<APlayerCharacter>(Actor);

        if (!IsValid(Character)) continue;

        APlayerController* PC = Cast<APlayerController>(Character->GetController());

        if (!IsValid(PC)) continue;

        if (PC->GetPawn() != Character) continue;

        if (!TriggerBox->IsOverlappingComponent(Character->GetCapsuleComponent())) continue;

        CurrentPlayers.Add(PC);
    }

    bool bSamePlayers = CurrentPlayers.Num() == PlayersInZone.Num();

    if (bSamePlayers)
    {
        for (const TWeakObjectPtr<APlayerController>& PC : CurrentPlayers)
        {
            if (!PlayersInZone.Contains(PC))
            {
                bSamePlayers = false;
                break;
            }
        }
    }

    PlayersInZone = MoveTemp(CurrentPlayers);

    const int32 RequiredPlayers = GameMode->GetRequiredPlayers();

    const bool bAllPlayersReady = PlayersInZone.Num() == RequiredPlayers && GameMode->GetNumPlayers() == RequiredPlayers;

    if (!bAllPlayersReady)
    {
        ReadyStartTime = -1.0;
        return;
    }

    const double CurrentTime = World->GetTimeSeconds();

    if (ReadyStartTime < 0.0 || !bSamePlayers)
    {
        ReadyStartTime = CurrentTime;

        UE_LOG(LogTemp, Warning, TEXT("[Zone] Countdown Started"));

        return;
    }

    const double ElapsedTime = CurrentTime - ReadyStartTime;

    UE_LOG(LogTemp, Warning, TEXT("[Zone] Elapsed: %.2f / %.2f"), ElapsedTime, HoldSeconds);

    if (ElapsedTime < HoldSeconds) return;

    UE_LOG(LogTemp, Warning, TEXT("[Zone] Travel Started"));

    if (GameMode->StartGameLevel())
    {
        bTransitionRequested = true;

        GetWorldTimerManager().ClearTimer(ZoneCheckTimer);
    }
}

// Called every frame
void ALevelTransitionZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

