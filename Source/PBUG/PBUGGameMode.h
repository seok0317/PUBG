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

	// 플레이어가 서버에 성공적으로 접속했을 때 호출됨
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어가 사망했을 때 캐릭터에서 호출할 함수
	void PlayerDied(AController* VictimController);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBPC_ItemSpawner* ItemSpawner; // 컴포넌트 선언
};



