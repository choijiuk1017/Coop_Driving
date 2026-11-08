// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerRoleComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Player/Disabilities/Deaf.h"
#include "Player/Disabilities/Mute.h"
#include "Online/GameplayVoiceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
// Sets default values for this component's properties
UPlayerRoleComponent::UPlayerRoleComponent()
{

	PrimaryComponentTick.bCanEverTick = false;
    DeafEffect = CreateDefaultSubobject<UDeaf>(TEXT("DeafEffect"));
    MuteEffect = CreateDefaultSubobject<UMute>(TEXT("MuteEffect"));
    SetIsReplicatedByDefault(true);             //���ø����̼� ����
}


// Called when the game starts
void UPlayerRoleComponent::BeginPlay()
{
	Super::BeginPlay();    

}

void UPlayerRoleComponent::AssignRole(EDrivingRole NewRole)
{
    AActor* Owner = GetOwner();

    if (!Owner || !Owner->HasAuthority())
    {
        return;
    }

    TArray<int32> NewDisabilityIds;

    /*
                
       [���]  [ID]	 [�����ϴ� ���]	
     �þ� ���	1	������ �� �� ����	
     û�� ���	2	�ٸ� �÷��̾��� �ΰ��� ���̽��� ���� �� ����
     ��ȭ ���	3	�ΰ��� ���̽��� ���� �� ����


     ���� ��ȹ�̳� ��Ȳ�� �°� ���� ��չ�ȣ AddUnique�ϸ� ��. �� ���� ���ҿ� ���� ��չ�ȣ �߰���X 
    */
    switch (NewRole)
    {
    case EDrivingRole::None:
        break;

    case EDrivingRole::Briefing:
        NewDisabilityIds.AddUnique(2);
        NewDisabilityIds.AddUnique(3);
        break;

    case EDrivingRole::Steering:
        NewDisabilityIds.AddUnique(2);
        break;
    case EDrivingRole::Pedals:
        NewDisabilityIds.AddUnique(1);
        NewDisabilityIds.AddUnique(3);
        NewDisabilityIds.AddUnique(2);
        break;

    case EDrivingRole::Gear:
        NewDisabilityIds.AddUnique(3);
        break;

    default:
        return;
    }

    Assignment.Role = NewRole;
    Assignment.DisabilityIds = NewDisabilityIds;
    PrintAssignmentDebug();
    RefreshLocalVoiceEffects(); // Listen host does not receive C++ OnRep.
}

void UPlayerRoleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UPlayerRoleComponent, Assignment);
}

void UPlayerRoleComponent::OnRep_Assignment()
{
    RefreshLocalVoiceEffects();
    /*UE_LOG(
        LogTemp,
        Log,
        TEXT("[Role] Owner=%s Role=%d DisabilityCount=%d"),
        *GetNameSafe(GetOwner()),
        static_cast<int32>(Assignment.Role),
        Assignment.DisabilityIds.Num());*/
    PrintAssignmentDebug();
}

void UPlayerRoleComponent::RefreshLocalVoiceEffects()
{
    if (Assignment.Role == EDrivingRole::None || !GetWorld() || !GetWorld()->GetGameInstance()) return;
    APlayerController* PC = GetWorld()->GetGameInstance()->GetFirstLocalPlayerController();
    if (!PC || !PC->IsLocalController() || PC->PlayerState.Get() != GetOwner()) return;
    if (bVoiceEffectsApplied && LastVoiceController.Get() == PC && LastVoiceDisabilityIds == Assignment.DisabilityIds) return;

    if (Assignment.DisabilityIds.Contains(2)) DeafEffect->ApplyLocal(PC);
    else DeafEffect->RemoveLocal(PC);
    if (Assignment.DisabilityIds.Contains(3)) MuteEffect->ApplyLocal(PC);
    else MuteEffect->RemoveLocal(PC);

    LastVoiceController = PC;
    LastVoiceDisabilityIds = Assignment.DisabilityIds;
    bVoiceEffectsApplied = true;
    PC->GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->RefreshPolicy();
    UE_LOG(LogTemp, Log, TEXT("[Voice] Local role policy: %s Role=%d Deaf=%d Mute=%d"),
        *GetNameSafe(GetOwner()), static_cast<int32>(Assignment.Role),
        Assignment.DisabilityIds.Contains(2), Assignment.DisabilityIds.Contains(3));
}


//�ش� �Լ��� ������������ ���߿� �����Ұ���
void UPlayerRoleComponent::PrintAssignmentDebug() const
{
    const AActor* Owner = GetOwner();

    if (!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }



    TArray<FString> DisabilityNames;

    for (int32 Id : Assignment.DisabilityIds)
    {
        switch (Id)
        {
        case 1:
            DisabilityNames.Add(TEXT("Blind(1)"));
            break;
        case 2:
            DisabilityNames.Add(TEXT("Deaf(2)"));
            break;
        case 3:
            DisabilityNames.Add(TEXT("Mute(3)"));
            break;
        default:
            DisabilityNames.Add(
                FString::Printf(TEXT("Unknown(%d)"), Id));
            break;
        }
    }

    const FString DisabilityText = DisabilityNames.IsEmpty()
        ? TEXT("None")
        : FString::Join(DisabilityNames, TEXT(", "));

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[Role][%s] Owner=%s Role=%d Disabilities=[%s]"),
        Owner->HasAuthority() ? TEXT("Server") : TEXT("Client"),
        *Owner->GetName(),
        static_cast<int32>(Assignment.Role),
        *DisabilityText);
}
