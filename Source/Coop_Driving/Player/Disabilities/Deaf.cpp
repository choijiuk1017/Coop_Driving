// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Disabilities/Deaf.h"
#include "Online/GameplayVoiceSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

UDeaf::UDeaf() { DisabilityId = 2; }

void UDeaf::ApplyLocal_Implementation(APlayerController* PC)
{
    if (IsValid(PC) && PC->IsLocalController())
        PC->GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->SetDeaf(true);
}

void UDeaf::RemoveLocal_Implementation(APlayerController* PC)
{
    if (IsValid(PC) && PC->IsLocalController())
        PC->GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->SetDeaf(false);
}

