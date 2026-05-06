#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h"
#include "BPC_ConsumableComponent.generated.h"

// UI에 사용 시간을 알려주기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUseItemStarted, FName, ItemID, float, UseTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseItemStopped);
// 부스트 게이지 업데이트를 알릴 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoostUpdated, float, CurrentBoost);

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

	// 현재 부스트 게이지 (0 ~ 100, 복제 필요)
	UPROPERTY(ReplicatedUsing = OnRep_BoostChanged, BlueprintReadOnly, Category = "Consumable")
	float CurrentBoost = 0.0f;

	UFUNCTION()
	void OnRep_BoostChanged();

	UPROPERTY(BlueprintAssignable, Category = "Consumable|Events")
	FOnBoostUpdated OnBoostUpdated;

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

	// 부스트에 의한 자동 회복 로직
	void HandleBoostLogic(float DeltaTime);

	// 부스트 업데이트를 위한 타이머 핸들
	FTimerHandle BoostUpdateTimerHandle;

	// 0.1초마다 실행될 로직 함수
	void UpdateBoostLoop();

	// 업데이트 간격 설정 (0.1초)
	const float BoostUpdateInterval = 0.1f;

	// 8초 주기를 체크하기 위한 누적 시간 변수
	float BoostHealAccumulator = 0.0f;

	// 배그 기준 기본 이동 속도 (필요시 캐릭터에서 가져오도록 수정 가능)
	const float BaseMoveSpeed = 500.0f;
};