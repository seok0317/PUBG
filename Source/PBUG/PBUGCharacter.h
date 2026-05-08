// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InteractionInterface.h" 
#include "BPC_Inventory.h"
#include "GameplayTagContainer.h"
#include "PBUGCharacter.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);

UCLASS(config=Game)
class APBUGCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	// / Aim Input Action
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AimAction;

	/** Equip Slot 1 Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* EquipSlot1Action;

	/** Equip Slot 2 Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* EquipSlot2Action;

	/** Holster Weapon Input Action (무기 집어넣기) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* HolsterAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchFireModeAction;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChanged OnHealthChanged;

	AActor* LastTarget;

public:
	APBUGCharacter();
	


protected:

	FTimerHandle InteractTimerHandle;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
	void CheckInteractables();

	// 1번 무기 꺼내기
	void EquipSlot1(const FInputActionValue& Value);

	// 2번 무기 꺼내기
	void EquipSlot2(const FInputActionValue& Value);

	// 무기 집어넣기 (맨손)
	void HolsterWeapon(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	// / 발사 버튼 입력 액션
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** 발사 버튼을 눌렀을 때 실행될 핸들러 함수 */
	void Fire(const FInputActionValue& Value);

	void Reload(const FInputActionValue& Value);

	// 입력과 연결될 함수
	void Interact(const FInputActionValue& Value);

	// 현재 체력 (RepNotify를 사용하여 UI 자동 갱신)
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Stat")
	float CurrentHealth = 100.0f;

	// 최대 체력 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
	float MaxHealth = 100.0f;

	// 1. 사망 상태 변수 (복제 설정)
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	// 2. 사망 시 클라이언트에서 실행될 비주얼 함수
	UFUNCTION()
	void OnRep_IsDead();

	// 3. 내부 사망 처리 로직
	void HandleDeathVisuals();


	// 자동 갱신 할때 실행되는 함수
	UFUNCTION()
	void OnRep_CurrentHealth();

	void Die();

	// 조준 입력과 연결될 함수
	void StartAim(const FInputActionValue& Value);
	void StopAim(const FInputActionValue& Value);

	// 블루프린트에서 구현할 조준 애니메이션 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void K2_StartADSEffects();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void K2_StopADSEffects();

	// 틱 대신 마우스 입력 시에만 업데이트될 복제 변수
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Combat")
	FRotator RemoteAimRotation;

	/*============== 연사 =================*/

	// 사격 중단을 위한 함수 추가
	void StopFire(const FInputActionValue& Value);

	// 사격 모드 전환을 위한 함수 추가
	void SwitchFireMode(const FInputActionValue& Value);

	/*============= Tag ==============*/

	UPROPERTY(ReplicatedUsing = OnRep_ActiveGameplayTags, VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	FGameplayTagContainer ActiveGameplayTags;

	// 기본 이동 속도 정의
	const float NormalWalkSpeed = 500.0f;
	const float SlowWalkSpeed = 150.0f; // 조준 및 아이템 사용 시 속도


public:
	// 서버로 각도를 보내는 RPC (사격만큼 중요하지 않으므로 Unreliable 권장)
	UFUNCTION(Server, Unreliable)
	void Server_SetAimRotation(FRotator NewRot);

	// 태크바뀔때 OnRep함수
	UFUNCTION()
	void OnRep_ActiveGameplayTags();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 데미지 받기
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 체력 회복 함수 (MaxHealth를 넘지 않도록 처리)
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void Heal(float HealAmount, float MaxLimit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	AActor* TargetActor;

	// 인벤토리 컴포넌트 장착
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	class UBPC_Inventory* InventoryComponent;

	// 장비 컴포넌트 장착
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	class UBPC_Equipment* EquipmentComponent;

	// 소비 아이템 컴포넌트 장착
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Consumable")
	class UBPC_ConsumableComponent* ConsumableComponent;

	// F키를 눌렀을 때 실행할 함수
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void PickupItem();

	// 서버에서 실행될 진짜 줍기 함수 (RPC)
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Interaction")
	void Server_PickupItem(AActor* ItemToPickup);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnTargetItemChanged(AActor* NewTarget);

	// 일반 아이템 버리기
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void Server_DropItem(FName ItemID, int32 Quantity);

	// 무기 버리기
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory")
	void Server_DropWeapon(int32 SlotIndex);

	// 위 아래 조준 각도 가져오는 함수
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAimPitch() const;

	// 다른 곳에서 각도를 읽어갈 Getter
	FRotator GetRemoteAimRotation() const { return RemoteAimRotation; }

	UFUNCTION(BlueprintPure, Category = "Stat")
	float GetCurrentHealth() const { return CurrentHealth; }

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/*========================== Tag ==========================*/

	// 태그 추가
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void AddGameplayTag(FGameplayTag TagToAdd);

	// 태그 제거
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveGameplayTag(FGameplayTag TagToRemove);

	// 태그 확인 (정확히 일치하는지)
	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool HasMatchingTag(FGameplayTag TagToCheck) const;

	
	 // 계층 구조 체크 (State.Action 하위 태그가 하나라도 있는지 확인할 때 사용)
	bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;


	// --- 기존 블루프린트 노드 유지용 브릿지 함수 ---
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsDead() const;
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsAiming() const;
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsReloading() const;
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsUsingItem() const;
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsArmed() const;

	// 캐릭터의 현재 상태(태그)를 확인하여 속도를 새로고침하는 함수
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RefreshMovementSpeed();
};

