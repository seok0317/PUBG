// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBUGGameMode.h"
#include "PBUGCharacter.h"
#include "PBUGGameState.h"
#include "UObject/ConstructorHelpers.h"

APBUGGameMode::APBUGGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
		ItemSpawner = CreateDefaultSubobject<UBPC_ItemSpawner>(TEXT("ItemSpawner"));
	}
}

void APBUGGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 아이템 스폰 실행
	if (ItemSpawner)
	{
		ItemSpawner->InitializeItemSpawning();
	}
}

void APBUGGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // 게임 스테이트를 가져와 인원수 증가
    APBUGGameState* GS = GetGameState<APBUGGameState>();
    if (GS)
    {
        GS->AlivePlayerCount++;
        // 서버는 OnRep이 자동으로 안 돌기 때문에 수동으로 방송해줌 (서버 UI용)
        GS->OnRep_AlivePlayerCount();

        UE_LOG(LogTemp, Warning, TEXT("New Player Joined! Total: %d"), GS->AlivePlayerCount);
    }
}

void APBUGGameMode::PlayerDied(AController* VictimController)
{
    APBUGGameState* GS = GetGameState<APBUGGameState>();
    if (GS && GS->AlivePlayerCount > 0)
    {
        GS->AlivePlayerCount--;
        GS->OnRep_AlivePlayerCount();

        UE_LOG(LogTemp, Warning, TEXT("Player Died! Remaining: %d"), GS->AlivePlayerCount);
    }
}