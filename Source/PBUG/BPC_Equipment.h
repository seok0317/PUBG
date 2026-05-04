#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemData.h"
#include "BPC_Equipment.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAmmoUpdated);

class ABulletProjectile;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PBUG_API UBPC_Equipment : public UActorComponent
{
	GENERATED_BODY()

public:
	UBPC_Equipment();

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 현재 들고 있는 무기의 잔탄수와 최대 탄창 용량을 가져오는 헬퍼 함수
	void GetCurrentAmmoData(int32& OutCurrentAmmo, int32& OutMaxAmmo, FName& OutAmmoID);


public:

	// 에디터 디테일 패널에서 우리가 만든 BP_Bullet을 선택할 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<class ABulletProjectile> BulletClass;

	// [추가] 장비가 변경되었을 때 블루프린트 UI에 알려줄 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentUpdated OnEquipmentUpdated;


	/* ==================== 1번 무기 ==================== */
	UPROPERTY(ReplicatedUsing = OnRep_MainWeapon1, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FSlotData MainWeapon1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	AActor* MainWeapon1Actor;

	UFUNCTION()
	void OnRep_MainWeapon1();

	/* ==================== 2번 무기 ==================== */
	// [추가] 2번 슬롯 데이터 및 액터
	UPROPERTY(ReplicatedUsing = OnRep_MainWeapon2, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FSlotData MainWeapon2;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	AActor* MainWeapon2Actor;

	UFUNCTION()
	void OnRep_MainWeapon2();

	/* ================================================== */

	// 서버에서 무기를 장착하는 함수
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipWeapon(FName ItemID);

	// 현재 손에 들고 있는 무기의 슬롯 번호 (0: 맨손, 1: 1번 무기, 2: 2번 무기)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeaponIndex, BlueprintReadOnly, Category = "Equipment")
	int32 CurrentWeaponIndex = 0;

	UFUNCTION()
	void OnRep_CurrentWeaponIndex();

	// 클라이언트가 서버에게 1, 2, X번 키를 눌렀다고 알리는 RPC
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Equipment")
	void Server_EquipWeaponIndex(int32 SlotIndex);

	// 데이터 테이블 참조용 (캐릭터나 인벤토리에서 가져옴)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	UDataTable* ItemDataTable;

	// 멀티플레이 필수 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버 발사 RPC
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Weapon")
	void Server_Fire(FVector MuzzleLocation, FRotator FireRotation);


	// 모든 클라이언트에게 이펙트 방송
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffects();

	// 무기 버리기 (SlotIndex: 1 또는 2)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void DropWeapon(int32 SlotIndex);

	// 조준 상태 변수와 서버 RPC
	UPROPERTY(ReplicatedUsing = OnRep_IsAiming, BlueprintReadOnly, Category = "Combat")
	bool bIsAiming = false;

	UFUNCTION()
	void OnRep_IsAiming();

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Combat")
	void Server_SetAiming(bool bNewAiming);

	/* ==================== 조준 ==================== */

	// 블루프린트에서 카메라 줌 수치를 가져가기 위한 함수
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetADSFOV();


	 //조준 애니메이션 블루프린트에서 가져다 쓸 최종 좌표
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void GetADSHandTransform(FVector& OutLocation, FRotator& OutRotation);

	// [추가] 위치 보정값 (X: 앞뒤, Y: 좌우, Z: 위아래)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|ADS")
	FVector ADSLocationOffset = FVector::ZeroVector;

	// [기존] 회전 보정값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|ADS")
	FRotator ADSRotationOffset = FRotator(0.0f, 180.0f, 0.0f);

	/* ===================== 연사 ======================= */

	// 현재 선택된 사격 모드 (복제 필요)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	EFireMode CurrentFireMode = EFireMode::Single;

	// 사격 모드 전환 서버 RPC
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Weapon")
	void Server_SwitchFireMode();

	// 사격 시작 및 중지 (캐릭터에서 호출함)
	void StartFire();
	void StopFire();

	/* ===================== 반동 ======================= */

	void ApplyRecoil();

	// 반동 복구 초기화
	void ResetRecoilRecovery() { PendingRecoilRecovery = 0.0f; }

	// 아직 복구되지 않고 남아있는 반동 수치 
	float PendingRecoilRecovery = 0.0f;

	// 플레이어가 마우스를 움직이고 있는지 체크 (마우스 조작 중엔 복구 일시 정지)
	bool bIsUserMovingMouse = false;

	/* ===================== 재장전 ======================= */

	// 1번 슬롯 무기의 현재 잔탄수 (복제)
	UPROPERTY(ReplicatedUsing = OnRep_AmmoChanged, BlueprintReadOnly, Category = "Equipment")
	int32 AmmoInMag1;

	// 2번 슬롯 무기의 현재 잔탄수 (복제)
	UPROPERTY(ReplicatedUsing = OnRep_AmmoChanged, BlueprintReadOnly, Category = "Equipment")
	int32 AmmoInMag2;

	// 재장전 요청 서버 RPC
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Weapon")
	void Server_Reload();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnAmmoUpdated OnAmmoUpdated;

	UFUNCTION()
	void OnRep_AmmoChanged();

	// 현재 들고 있는 무기의 [현재 탄창 / 예비 탄수]를 한 번에 가져오는 함수
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void GetCurrentWeaponAmmoInfo(int32& CurrentMag, int32& TotalExtra);

	// 장전 애니메이션 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	bool bIsReloading = false;

	// 애니메이션 재생을 위한 멀티캐스트 RPC
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayReloadAnim(UAnimMontage* MontageToPlay);

private:
	// 실제로 총을 소환해서 붙이는 내부 함수
	void SpawnWeaponVisual(int32 SlotIndex);

	// 실제로 무기를 등에서 손으로, 손에서 등으로 옮기는 함수
	void UpdateWeaponAttachment();

	// 연사 타이머 핸들
	FTimerHandle FireTimerHandle;

	// 실제로 발사 로직(조준 보정 + Server_Fire)을 실행하는 함수
	void FireTick();

	// 재장전 처리를 완료하는 함수 (타이머용)
	void FinishReloading();
};