#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h"
#include "BPC_ItemSpawner.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PBUG_API UBPC_ItemSpawner : public UActorComponent
{
    GENERATED_BODY()

public:
    UBPC_ItemSpawner();

    // 서버 시작 시 아이템을 뿌립니다. (GameMode에서 호출 예정)
    UFUNCTION(BlueprintCallable, Category = "ItemSpawn")
    void InitializeItemSpawning();

protected:
    // 실제 아이템을 소환하는 함수
    void SpawnItemBundle(FVector Location);

    // 랜덤 아이템 ID를 가져오는 함수
    FName GetRandomItemID();

    UPROPERTY(EditAnywhere, Category = "ItemSpawn")
    class UDataTable* ItemDataTable;

    UPROPERTY(EditAnywhere, Category = "ItemSpawn")
    TSubclassOf<class AItemBase> ItemBaseClass;

    // 아이템이 스폰될 확률 (0.0 ~ 1.0)
    UPROPERTY(EditAnywhere, Category = "ItemSpawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnChance = 0.8f;;
};