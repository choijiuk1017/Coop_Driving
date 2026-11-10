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

USTRUCT(BlueprintType)
struct FRoleAssignment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Role")
	EDrivingRole Role = EDrivingRole::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Role")
	TArray<int32> DisabilityIds;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COOP_DRIVING_API UPlayerRoleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerRoleComponent();
	void AssignRole(EDrivingRole NewRole);

    UFUNCTION(BlueprintPure, Category = "Role")
    EDrivingRole GetRole() const { return Assignment.Role; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	void PrintAssignmentDebug() const;
	virtual void BeginPlay() override;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		ReplicatedUsing = OnRep_Assignment,
		Category = "Role")					//상태 레플리케이션과 할당 콜백함수 등록
	FRoleAssignment Assignment;

	UFUNCTION()
	void OnRep_Assignment();
};
