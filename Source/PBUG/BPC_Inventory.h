#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h" // 방금 만든 구조체 사용을 위해 포함
#include "BPC_Inventory.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PBUG_API UBPC_Inventory : public UActorComponent
{
	GENERATED_BODY()

public:
	UBPC_Inventory();

protected:
	virtual void BeginPlay() override;

public:
    //블루프린트에서 호출을 들을 수 있는 이벤트 변수 생성
    UPROPERTY(BlueprintAssignable, Category = "Inventory")
    FOnInventoryUpdated OnInventoryUpdated;

	// 인벤토리 실제 내용물 (배열)
    UPROPERTY(ReplicatedUsing = OnRep_InventoryArray, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    TArray<FSlotData> InventoryArray;

    // 현재 가방에 찬 총 무게
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    float CurrentWeight = 0.0f;

    // 현재 가방의 최대 용량 (기본 용량 + 가방 보너스)
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
    float MaxCapacity = 50.0f; // 가방 없을 때 기본 용량

    // 데이터 테이블 변수 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    class UDataTable* ItemDataTable;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DebugPrintInventory();

    // 데이터가 도착했을 때 실행될 함수 선언
    UFUNCTION()
    void OnRep_InventoryArray();

    // 아이템 추가 시 무게 체크 로직
    bool CanAddItem(float WeightToAdd) {
        return (CurrentWeight + WeightToAdd) <= MaxCapacity;
    }

    // 멀티플레이를 위한 필수 함수
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 가방 장착 시 용량 업데이트 함수
    void UpdateMaxCapacity(float NewBonus) {
        MaxCapacity = 50.0f + NewBonus; // 기본 50에 가방 수치 더하기
    }

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void DropItem(FName ItemID, int32 Quantity);

    // 공용 헬퍼: 바닥에 아이템 액터 생성 (무기 컴포넌트에서도 호출 가능하도록 public)
    void SpawnItemOnGround(FName ItemID, int32 Quantity, int32 InitialAmmo = 0);

	// 아이템 추가 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddToInventory(FName ID, int32 Amount);

    // 주변 아이템을 찾아서 배열로 반환 (블루프린트 UI에서 사용)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    TArray<AActor*> GetVicinityItems(float Radius = 300.0f);

    // 에디터에서 만든 BP_ItemBase를 여기에 넣어줄 겁니다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TSubclassOf<class AItemBase> ItemBaseClass;

    int32 InAmmo = 0;

    /* ==================== 아이템 소모 ================== */

    // 특정 아이템의 총 개수 확인
    int32 GetTotalQuantityByID(FName ItemID);

    // 특정 아이템을 지정된 수량만큼 제거 (재장전용)
    void ConsumeItem(FName ItemID, int32 Quantity);

    // 가방 배열에서 실제 데이터 제거 로직
    void RemoveItemData(FName ItemID, int32 Quantity);
private: 
    
};