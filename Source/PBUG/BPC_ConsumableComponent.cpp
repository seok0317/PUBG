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

	PC->AddGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Action.UsingItem")));
	bIsUsing = true;

	Client_NotifyUseItem(ItemID, Data->UseTime);

	// 2. 타이머 시작 (서버에서 시간 체크)
	GetWorld()->GetTimerManager().SetTimer(UseItemTimerHandle, this, &UBPC_ConsumableComponent::FinishUseItem, Data->UseTime, false);

	
}

void UBPC_ConsumableComponent::Client_NotifyUseItem_Implementation(FName ItemID, float UseTime)
{
	// 이 함수가 실행되는 시점은 클라이언트의 컴퓨터입니다.
	// 여기서 Broadcast를 해야 블루프린트 UI가 소리를 듣고 작동합니다.
	OnUseItemStarted.Broadcast(ItemID, UseTime);

	// [덤] 클라이언트도 즉시 속도를 줄여서 버벅임을 방지합니다. (예측)
	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	if (PC)
	{
		PC->RefreshMovementSpeed();
	}
}


void UBPC_ConsumableComponent::CancelUsing()
{
	GetWorld()->GetTimerManager().ClearTimer(UseItemTimerHandle);

	if (bIsUsing)
	{
		bIsUsing = false;
		APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
		if (PC) PC->RemoveGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Action.UsingItem")));;
		Client_NotifyCancelItem();
	}
}

void UBPC_ConsumableComponent::Client_NotifyCancelItem_Implementation()
{
	OnUseItemStopped.Broadcast();
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
	if (!Character) return;

	Character->RefreshMovementSpeed();

	
	// 6. [핵심] 8초 주기 회복 로직
	float HealPer8Sec = 0.0f;
	if (CurrentBoost > 90.0f) HealPer8Sec = 4.0f;
	else if (CurrentBoost > 60.0f) HealPer8Sec = 3.0f;
	else if (CurrentBoost > 20.0f) HealPer8Sec = 2.0f;
	else HealPer8Sec = 1.0f;

	BoostHealAccumulator += BoostUpdateInterval;
	if (BoostHealAccumulator >= 8.0f)
	{
		Character->Heal(HealPer8Sec, 100.0f);
		BoostHealAccumulator = 0.0f;
	}

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