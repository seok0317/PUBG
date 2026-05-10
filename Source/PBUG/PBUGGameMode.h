// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BPC_ItemSpawner.h" 
#include "PBUGGameMode.generated.h"

UCLASS(minimalapi)
class APBUGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APBUGGameMode();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBPC_ItemSpawner* ItemSpawner; // 컴포넌트 선언
};



