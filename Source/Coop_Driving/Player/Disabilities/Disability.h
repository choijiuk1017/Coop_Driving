#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Disability.generated.h"
class APlayerController;
UCLASS(
    Abstract,
    BlueprintType,
    Blueprintable,
    EditInlineNew,
    DefaultToInstanced)
    class COOP_DRIVING_API UDisability : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Disability")
    int32 GetDisabilityId() const
    {
        return DisabilityId;
    }

    UFUNCTION(BlueprintNativeEvent, Category = "Disability")
    void ApplyLocal(APlayerController* PlayerController);

    virtual void ApplyLocal_Implementation(
        APlayerController* PlayerController);

    UFUNCTION(BlueprintNativeEvent, Category = "Disability")
    void RemoveLocal(APlayerController* PlayerController);

    virtual void RemoveLocal_Implementation(
        APlayerController* PlayerController);


protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Disability")
    int32 DisabilityId = 0;
};