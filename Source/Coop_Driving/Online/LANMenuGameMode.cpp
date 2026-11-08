#include "Online/LANMenuGameMode.h"
#include "Online/LANSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "CoopLANMenu"
ALANMenuGameMode::ALANMenuGameMode()
{
    PlayerControllerClass = ALANMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
}

void ALANMenuPlayerController::LANConnect()
{
    GetGameInstance()->GetSubsystem<ULANSessionSubsystem>()->ConnectOrHost();
}

void ALANMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController() || !GetGameInstance()->GetGameViewportClient()) return;
    const TWeakObjectPtr<ULANSessionSubsystem> Service = GetGameInstance()->GetSubsystem<ULANSessionSubsystem>();
    MenuWidget = SNew(SOverlay)
        + SOverlay::Slot()
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.018f, 0.025f, 0.04f, 1.0f))
        ]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(32.0f, 48.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
            [
                SNew(STextBlock).Text(LOCTEXT("Title", "COOP DRIVING"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
            [
                SAssignNew(ConnectButton, SButton).ContentPadding(FMargin(48, 16))
                .IsEnabled_Lambda([Service]() { return Service.IsValid() && !Service->IsBusy(); })
                .OnClicked_Lambda([Service]()
                {
                    if (Service.IsValid()) Service->ConnectOrHost();
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock).Text(LOCTEXT("Connect", "접속/방생성"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 20, 0, 0)
            [
                SNew(STextBlock)
                .Text_Lambda([Service]() { return Service.IsValid() ? Service->GetStatus() : FText::GetEmpty(); })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                .AutoWrapText(true)
            ]
        ];
    GetGameInstance()->GetGameViewportClient()->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 100);
    bShowMouseCursor = true;
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(ConnectButton);
    SetInputMode(InputMode);
}

void ALANMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (MenuWidget.IsValid() && GetGameInstance() && GetGameInstance()->GetGameViewportClient())
    {
        GetGameInstance()->GetGameViewportClient()->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    }
    MenuWidget.Reset();
    ConnectButton.Reset();
    Super::EndPlay(Reason);
}
#undef LOCTEXT_NAMESPACE
