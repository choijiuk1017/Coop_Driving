// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"


class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 
 */
UCLASS()
class COOP_DRIVING_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AMainPlayerController();
    virtual bool InputKey(const FInputKeyParams& Params) override;

    UFUNCTION(Exec)
    void LANLeave();

    UFUNCTION(Exec)
    void LANVoiceStatus();

    virtual void ClientEnableNetworkVoice_Implementation(bool bEnable) override;
    virtual void ToggleSpeaking(bool bInSpeaking) override;


protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // Enhanced Input
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    // Input
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void StartJump();
    void StopJump();
    void LeaveLANSession();
};
