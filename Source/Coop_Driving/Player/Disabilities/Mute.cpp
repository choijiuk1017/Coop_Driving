// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Disabilities/Mute.h"
#include "Online/GameplayVoiceSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

UMute::UMute() { DisabilityId = 3; }

void UMute::ApplyLocal_Implementation(APlayerController* PC)
{
    if (IsValid(PC) && PC->IsLocalController())
        PC->GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->SetMute(true);
}

void UMute::RemoveLocal_Implementation(APlayerController* PC)
{
    if (IsValid(PC) && PC->IsLocalController())
        PC->GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->SetMute(false);
}

