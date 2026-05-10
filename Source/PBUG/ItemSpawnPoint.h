#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h" // 추가
#include "ItemSpawnPoint.generated.h"

UCLASS()
class PBUG_API AItemSpawnPoint : public AActor
{
    GENERATED_BODY()
public:
    AItemSpawnPoint() {
        PrimaryActorTick.bCanEverTick = false;

        // 루트 컴포넌트 설정
        USceneComponent* Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
        RootComponent = Scene;

        // 에디터에서 보일 아이콘(빌보드) 추가
        UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
        Billboard->SetupAttachment(RootComponent);

        // 에디터에서만 보이게 설정 (게임 중엔 안 보임)
        Billboard->bHiddenInGame = true;
    }

    UPROPERTY(EditAnywhere, Category = "Spawn Settings")
    bool bOnlyWeapon = false;
};