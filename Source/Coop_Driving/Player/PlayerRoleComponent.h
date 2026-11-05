// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerRoleComponent.generated.h"

UENUM(BlueprintType)
enum class EDrivingRole : uint8
{
	None     UMETA(DisplayName = "Unassigned"),
	Briefing UMETA(DisplayName = "Briefing"),
	Steering UMETA(DisplayName = "Steering"),
	Pedals   UMETA(DisplayName = "Pedals"),
	Gear     UMETA(DisplayName = "Gear")
};
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COOP_DRIVING_API UPlayerRoleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerRoleComponent();
	void AssignRole(EDrivingRole NewRole);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Role")
	EDrivingRole CurrentRole = EDrivingRole::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Role")
	int32 CurrentDisabilityId = 0;
		
};
