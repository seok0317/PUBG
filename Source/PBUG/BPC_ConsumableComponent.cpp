#include "BPC_ConsumableComponent.h"
#include "PBUGCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BPC_Inventory.h"
#include "Net/UnrealNetwork.h"

UBPC_ConsumableComponent::UBPC_ConsumableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBPC_ConsumableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBPC_ConsumableComponent, CurrentBoost);
}

void UBPC_ConsumableComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UBPC_ConsumableComponent::Server_StartUseItem_Validate(FName ItemID)
{
	return !ItemID.IsNone();
}

void UBPC_ConsumableComponent::Server_StartUseItem_Implementation(FName ItemID)
{
	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	// 무언가 행동 중이면 아이템 사용 불가
	if (!PC || PC->HasAnyMatchingGameplayTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("State.Action"))))) return;

	if (!ItemDataTable) return;

	FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));
	if (!Data) return;

	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	if (!Character) return;

	if (Data->ConsumableType == EConsumableType::Drink)
	{
		// 드링크는 체력과 상관없이 마실 수 있음. 
		// 단, 이미 부스트 게이지가 100이라면 마실 수 없게 설정 (선택 사항)
		if (CurrentBoost >= 100.0f)
		{
			UE_LOG(LogTemp, Warning, TEXT("이미 부스트 게이지가 가득 찼습니다."));
			return;
		}
	}
	// 2. 일반 회복 아이템(붕대, 구상 등)인 경우
	else
	{
		// 현재 체력이 아이템의 한계치(예: 75) 이상이면 사용 불가
		if (Character->GetCurrentHealth() >= Data->MaxHealLimit)
		{
			UE_LOG(LogTemp, Warning, TEXT("아이템의 회복 한계치에 도달하여 사용할 수 없습니다."));
			return;
		}
	}

	// 1. 상태 설정 및 속도 저하
	bIsUsing = true;
	PendingItemID = ItemID;
	SetMovementSpeed(true);

	PC->AddGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Action.UsingItem")));
	bIsUsing = true;

	// 2. 타이머 시작 (서버에서 시간 체크)
	GetWorld()->GetTimerManager().SetTimer(UseItemTimerHandle, this, &UBPC_ConsumableComponent::FinishUseItem, Data->UseTime, false);

	// 3. 클라이언트들에게 알림 (애니메이션이나 UI 출력용)
	OnUseItemStarted.Broadcast(ItemID, Data->UseTime);
}

void UBPC_ConsumableComponent::CancelUsing()
{
	GetWorld()->GetTimerManager().ClearTimer(UseItemTimerHandle);

	if (bIsUsing)
	{
		bIsUsing = false;
		SetMovementSpeed(false);
		APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
		if (PC) PC->RemoveGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Action.UsingItem")));
		OnUseItemStopped.Broadcast();
	}
}

void UBPC_ConsumableComponent::SetMovementSpeed(bool bIsSlow)
{
	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	if (Character && Character->GetCharacterMovement())
	{
		if (bIsSlow)
		{
			// 사용 시작 시에는 무조건 150으로 고정
			Character->GetCharacterMovement()->MaxWalkSpeed = 150.0f;
		}
		else
		{
			// 사용이 끝났을 때는 현재 부스트 게이지를 체크해서 속도를 결정
			float SpeedMultiplier = 1.0f;
			if (CurrentBoost > 90.0f) SpeedMultiplier = 1.062f;
			else if (CurrentBoost > 60.0f) SpeedMultiplier = 1.025f;

			Character->GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * SpeedMultiplier;
		}
	}
}

void UBPC_ConsumableComponent::HandleBoostLogic(float DeltaTime)
{
	// 1. 게이지 서서히 감소 (예: 1초에 1씩 감소)
	float DecreaseAmount = 1.0f * DeltaTime;
	CurrentBoost = FMath::Clamp(CurrentBoost - DecreaseAmount, 0.0f, 100.0f);

	// 2. 체력 회복 (부스트 1당 1초에 약 1의 체력 회복 - 수치 조절 가능)
	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	if (Character)
	{
		// 배그처럼 틱당 아주 미세하게 회복
		float HealPerTick = 1.0f * DeltaTime;
		Character->Heal(HealPerTick, 100.0f); // 부스트는 한계치 100까지 회복 가능
	}

	// 3. [디테일] 부스트 60 이상이면 이동 속도 버프
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = (CurrentBoost >= 60.0f) ? 600.0f : 500.0f;
	}

	// 클라이언트 UI 갱신을 위해 값 변경 알림 (RepNotify가 안 불리는 서버 본인용)
	if (GetNetMode() == NM_ListenServer) OnRep_BoostChanged();
}

void UBPC_ConsumableComponent::OnRep_BoostChanged()
{
	OnBoostUpdated.Broadcast(CurrentBoost);
}

void UBPC_ConsumableComponent::UpdateBoostLoop()
{
	// 서버에서만 로직 처리
	if (GetOwnerRole() < ROLE_Authority) return;

	// 1. 게이지가 다 떨어졌으면 타이머 종료
	if (CurrentBoost <= 0.0f)
	{
		CurrentBoost = 0.0f;

		// 버프 복구
		APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
		if (Character && Character->GetCharacterMovement())
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = 500.0f;
		}

		GetWorld()->GetTimerManager().ClearTimer(BoostUpdateTimerHandle);
		OnRep_BoostChanged();
		return;
	}

	// 2. 게이지 감소 
	float DecayRate = 0.33f;
	CurrentBoost -= (DecayRate * BoostUpdateInterval);

	// 3. 체력 회복 로직 (배그의 도트 힐링 재현)
	// 예: 게이지가 있으면 1초당 1의 체력 회복 -> 0.1초당 0.1 회복
	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	if (!Character || !Character->GetCharacterMovement()) return;

	// 4. [핵심] 게이지 구간별 데이터 설정 (배그 공식 기준)
	float HealPer8Sec = 0.0f;
	float SpeedMultiplier = 1.0f;

	if (CurrentBoost > 90.0f)      // 4단계 (91~100)
	{
		HealPer8Sec = 4.0f;
		SpeedMultiplier = 1.062f; // 6.2% 증가
	}
	else if (CurrentBoost > 60.0f) // 3단계 (61~90)
	{
		HealPer8Sec = 3.0f;
		SpeedMultiplier = 1.025f; // 2.5% 증가
	}
	else if (CurrentBoost > 20.0f) // 2단계 (21~60)
	{
		HealPer8Sec = 2.0f;
	}
	else                           // 1단계 (1~20)
	{
		HealPer8Sec = 1.0f;
	}

	// 5. 이동 속도 즉시 반영
	if (!bIsUsing)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * SpeedMultiplier;
	}

	// 6. [핵심] 8초 주기 회복 로직
	BoostHealAccumulator += BoostUpdateInterval; // 0.1초씩 누적

	if (BoostHealAccumulator >= 8.0f) // 8초가 되면
	{
		Character->Heal(HealPer8Sec, 100.0f); // 구간에 맞는 힐량 적용
		BoostHealAccumulator = 0.0f;          // 누적 시간 초기화
		UE_LOG(LogTemp, Log, TEXT("부스트 8초 주기 회복 실행: +%f HP"), HealPer8Sec);
	}

	// 7. UI 동기화
	OnRep_BoostChanged();
}


void UBPC_ConsumableComponent::FinishUseItem()
{
	if (!bIsUsing) return;

	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	FItemData* Data = ItemDataTable->FindRow<FItemData>(PendingItemID, TEXT(""));

	if (Character && Data)
	{
		if (Data->ConsumableType == EConsumableType::Drink)
		{
			// 1. 게이지 충전
			CurrentBoost = FMath::Clamp(CurrentBoost + Data->BoostAmount, 0.0f, 100.0f);
			OnRep_BoostChanged(); // UI 갱신 호출

			// 2. 타이머가 이미 돌아가고 있지 않다면 새로 시작 (중복 실행 방지)
			if (!GetWorld()->GetTimerManager().IsTimerActive(BoostUpdateTimerHandle))
			{
				GetWorld()->GetTimerManager().SetTimer(
					BoostUpdateTimerHandle,
					this,
					&UBPC_ConsumableComponent::UpdateBoostLoop,
					BoostUpdateInterval,
					true // 반복 실행
				);
			}
		}
		else
		{
			Character->Heal(Data->HealAmount, Data->MaxHealLimit);
		}

		UBPC_Inventory* Inv = Character->FindComponentByClass<UBPC_Inventory>();
		if (Inv) Inv->ConsumeItem(PendingItemID, 1);
	}
	CancelUsing();
}