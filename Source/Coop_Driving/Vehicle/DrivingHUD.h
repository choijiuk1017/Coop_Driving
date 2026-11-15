#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DrivingHUD.generated.h"

UCLASS()
class COOP_DRIVING_API ADrivingHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
