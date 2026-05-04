#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h"
#include "BPC_ConsumableComponent.generated.h"

// UI에 사용 시간을 알려주기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUseItemStarted, FName, ItemID, float, UseTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseItemStopped);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PBUG_API UBPC_ConsumableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBPC_ConsumableComponent();

protected:
	virtual void BeginPlay() override;

public:
	// 아이템 사용 시작 요청 (서버 RPC)
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Consumable")
	void Server_StartUseItem(FName ItemID);

	// 아이템 사용 취소
	UFUNCTION(BlueprintCallable, Category = "Consumable")
	void CancelUsing();

	UPROPERTY(BlueprintAssignable, Category = "Consumable|Events")
	FOnUseItemStarted OnUseItemStarted;

	UPROPERTY(BlueprintAssignable, Category = "Consumable|Events")
	FOnUseItemStopped OnUseItemStopped;

private:
	// 실제 아이템 효과 적용 (타이머 종료 시 호출)
	void FinishUseItem();

	// 움직임 속도 조절 헬퍼
	void SetMovementSpeed(bool bIsSlow);

	FTimerHandle UseItemTimerHandle;
	FName PendingItemID;

	// 원래 속도 저장용
	float OriginalMaxWalkSpeed = 500.0f;
	bool bIsUsing = false;

	UPROPERTY(EditAnywhere, Category = "Consumable")
	class UDataTable* ItemDataTable;
};