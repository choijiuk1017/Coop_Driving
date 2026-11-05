// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerRoleComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UPlayerRoleComponent::UPlayerRoleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
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

    int32 NewDisabilityId = 0;

    switch (NewRole)
    {
    case EDrivingRole::None:
        NewDisabilityId = 0;
        break;

    case EDrivingRole::Briefing:
        NewDisabilityId = 2;
        break;

    case EDrivingRole::Steering:
    case EDrivingRole::Pedals:
        NewDisabilityId = 1;
        break;

    case EDrivingRole::Gear:
        NewDisabilityId = 3;
        break;

    default:
        return;
    }

    CurrentRole = NewRole;
    CurrentDisabilityId = NewDisabilityId;
}
