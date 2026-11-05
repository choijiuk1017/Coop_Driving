#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CoopPlayerState.generated.h"
class UPlayerRoleComponent;
/**
 * 
 */
UCLASS()
class COOP_DRIVING_API ACoopPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	ACoopPlayerState();
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Role")
	TObjectPtr<UPlayerRoleComponent> PlayerRoleComponent;
};
