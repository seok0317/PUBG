#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractionInterface : public UInterface 
{ 
    GENERATED_BODY() 
};

class PBUG_API IInteractionInterface
{
    GENERATED_BODY()

public:
	// This function will be called when the player interacts with an object that implements this interface.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void GetInteractInfo(FText& OutName, FText& OutAction);
};