// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBUGGameMode.h"
#include "PBUGCharacter.h"
#include "UObject/ConstructorHelpers.h"

APBUGGameMode::APBUGGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
