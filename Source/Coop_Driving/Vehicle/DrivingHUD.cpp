#include "Vehicle/DrivingHUD.h"
#include "Vehicle/DrivingPlayerController.h"
#include "Vehicle/CoopVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ADrivingHUD::DrawHUD()
{
    Super::DrawHUD();
    const ADrivingPlayerController* PC = Cast<ADrivingPlayerController>(GetOwningPlayerController());
    if (!PC || !Canvas || !GEngine) return;
    const float Scale = FMath::Clamp(Canvas->ClipX / 1000.f, 0.65f, 1.2f);
    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), 12.f, 12.f, Canvas->ClipX - 24.f, 155.f * Scale);
    TArray<FString> Lines;
    PC->GetDrivingHelp().ParseIntoArrayLines(Lines);
    float Y = 24.f;
    for (const FString& Line : Lines)
    {
        DrawText(Line, FLinearColor::White, 24.f, Y, GEngine->GetSmallFont(), Scale);
        Y += 23.f * Scale;
    }
    if (const ACoopVehicle* Vehicle = PC->GetSharedVehicle())
    {
        DrawText(FString::Printf(TEXT("Gear: %d | Brake: %.0f%%"),
            Vehicle->GetSelectedGear(), Vehicle->GetBrakePressure() * 100.f),
            FLinearColor::Yellow, 24.f, Y + 6.f, GEngine->GetSmallFont(), Scale);
    }
    else DrawText(TEXT("Waiting for shared vehicle..."), FLinearColor::Yellow, 24.f, Y + 6.f);
}
