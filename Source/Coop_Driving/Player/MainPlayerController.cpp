// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MainPlayerController.h"
#include "Player/PlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Online/LANSessionSubsystem.h"
#include "Online/GameplayVoiceSubsystem.h"

AMainPlayerController::AMainPlayerController()
{
	
}

void AMainPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;

    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

    if (Subsystem && DefaultMappingContext)
    {
        Subsystem->AddMappingContext(DefaultMappingContext, 0);
    }
}

void AMainPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput) return;

    EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainPlayerController::Move);
    EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainPlayerController::Look);
    EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AMainPlayerController::StartJump);
    EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMainPlayerController::StopJump);

}

void AMainPlayerController::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();

    APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn());

    if (PlayerCharacter)
    {
        PlayerCharacter->MoveCharacter(MovementVector);
    }
}

void AMainPlayerController::Look(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();

    AddYawInput(LookVector.X);
    AddPitchInput(LookVector.Y);
}

void AMainPlayerController::StartJump()
{
    APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn());

    if (PlayerCharacter)
    {
        PlayerCharacter->Jump();
    }
}

void AMainPlayerController::StopJump()
{
    APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetPawn());

    if (PlayerCharacter)
    {
        PlayerCharacter->StopJumping();
    }
}

void AMainPlayerController::LeaveLANSession()
{
    if (IsLocalController())
    {
        GetGameInstance()->GetSubsystem<ULANSessionSubsystem>()->LeaveSession();
    }
}

bool AMainPlayerController::InputKey(const FInputKeyParams& Params)
{
    if (Params.Key == EKeys::Escape && Params.Event == IE_Pressed)
    {
        LeaveLANSession();
        return true;
    }
    return Super::InputKey(Params);
}

void AMainPlayerController::LANLeave()
{
    LeaveLANSession();
}

void AMainPlayerController::ClientEnableNetworkVoice_Implementation(bool bEnable)
{
    // Keep engine handshakes from overriding the local open-mic/disability policy.
    ToggleSpeaking(bEnable);
}

void AMainPlayerController::ToggleSpeaking(bool bInSpeaking)
{
    // Open mic has no push-to-talk input: only the role policy controls transmission.
    GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->RefreshPolicy(true);
}

void AMainPlayerController::LANVoiceStatus()
{
    UE_LOG(LogTemp, Log, TEXT("[Voice] %s"),
        *GetGameInstance()->GetSubsystem<UGameplayVoiceSubsystem>()->GetDebugState());
}
