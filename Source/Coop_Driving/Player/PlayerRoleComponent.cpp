// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerRoleComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
// Sets default values for this component's properties
UPlayerRoleComponent::UPlayerRoleComponent()
{

	PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);             //레플리케이션 참여
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
                
       [결손]  [ID]	 [제한하는 기능]	
     시야 결손	1	전방을 볼 수 없음	
     청취 결손	2	다른 플레이어의 인게임 보이스를 들을 수 없음
     발화 결손	3	인게임 보이스로 말할 수 없음


     게임 기획이나 상황에 맞게 여러 결손번호 AddUnique하면 됌. 단 같은 역할에 같은 결손번호 추가는X 
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
}

void UPlayerRoleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UPlayerRoleComponent, Assignment);
}

void UPlayerRoleComponent::OnRep_Assignment()
{
    /*UE_LOG(
        LogTemp,
        Log,
        TEXT("[Role] Owner=%s Role=%d DisabilityCount=%d"),
        *GetNameSafe(GetOwner()),
        static_cast<int32>(Assignment.Role),
        Assignment.DisabilityIds.Num());*/
    PrintAssignmentDebug();
}


//해당 함수는 디버깅용임으로 나중에 삭제할것임
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