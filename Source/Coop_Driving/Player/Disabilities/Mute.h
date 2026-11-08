// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/Disabilities/Disability.h"
#include "Mute.generated.h"

/**
 * 
 */
UCLASS()
class COOP_DRIVING_API UMute : public UDisability
{
	GENERATED_BODY()
public:
    UMute();
    virtual void ApplyLocal_Implementation(APlayerController* PlayerController) override;
    virtual void RemoveLocal_Implementation(APlayerController* PlayerController) override;
	
};
