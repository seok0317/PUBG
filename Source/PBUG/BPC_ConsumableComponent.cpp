#include "BPC_ConsumableComponent.h"
#include "PBUGCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BPC_Inventory.h"
#include "Net/UnrealNetwork.h"

UBPC_ConsumableComponent::UBPC_ConsumableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

	if (Character->GetCurrentHealth() >= Data->MaxHealLimit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Your health is already high enough"));
		return;
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

void UBPC_ConsumableComponent::FinishUseItem()
{
	if (!bIsUsing) return;

	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	UBPC_Inventory* Inventory = Character->FindComponentByClass<UBPC_Inventory>();

	if (Character && Inventory)
	{
		FItemData* Data = ItemDataTable->FindRow<FItemData>(PendingItemID, TEXT(""));
		if (Data)
		{
			Character->Heal(Data->HealAmount, Data->MaxHealLimit);

			// 2. 인벤토리 아이템 소모
			Inventory->ConsumeItem(PendingItemID, 1);
		}
	}

	// 3. 상태 복구
	CancelUsing();
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
			// 사용 시작 시 속도 저장 후 150으로 감속
			OriginalMaxWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
			Character->GetCharacterMovement()->MaxWalkSpeed = 150.0f;
		}
		else
		{
			// 원래 속도로 복구
			Character->GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
		}
	}
}