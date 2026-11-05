// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CoopPlayerState.h"
#include "Player/PlayerRoleComponent.h"


ACoopPlayerState::ACoopPlayerState()
{
    PlayerRoleComponent =
        CreateDefaultSubobject<UPlayerRoleComponent>(
            TEXT("PlayerRoleComponent"));
}