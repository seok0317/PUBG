#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // 데이터 테이블 기능을 위해 반드시 필요!
#include "ItemData.generated.h"

UENUM(BlueprintType)
enum class EConsumableType : uint8
{
	Bandage UMETA(DisplayName = "Bandage"),        // 붕대 (찔끔 회복)
	FirstAidKit UMETA(DisplayName = "FirstAidKit"), // 구상 (75%까지 회복)
	MedKit UMETA(DisplayName = "MedKit"),         // 의료용 키트 (100% 회복)
	Drink UMETA(DisplayName = "Drink")             // 에너지 드링크
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single UMETA(DisplayName = "Single"), // 단발
	FullAuto UMETA(DisplayName = "FullAuto") // 연사
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None UMETA(DisplayName = "None"),
	// 무기
	Weapon UMETA(DisplayName = "Weapon"),
	// 소모품 (붕대, 음료수)
	Consumable UMETA(DisplayName = "Consumable"),
	// 장구류 (헬멧, 조끼)
	Equipment UMETA(DisplayName = "Equipment"),
	// 탄약
	Ammo UMETA(DisplayName = "Ammo")
};

// FItemData: 엑셀의 한 줄(Row)에 해당합니다.
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase // 중요: 데이터 테이블에서 쓰려면 반드시 상속받아야 함!
{
	GENERATED_BODY()

public:
	// 구조체 생성자 (초기값 설정)
	FItemData()
		: ItemName(FText::FromString("Unknown"))
		, ItemMesh(nullptr)
		, ItemIcon(nullptr)
		, ActionText(FText::FromString("Pick Up"))
		, ItemWeight(0.0f)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UStaticMesh* ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ActionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float ItemWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float CapacityBonus; // 가방일 경우 늘려줄 용량 (예: 1레벨 가방 150)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStackSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType;

	// 만약 무기라면, 등에 멜 때 어떤 액터를 생성할 것인가?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<AActor> WeaponClass;


	// 회복 아이템 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable")
	EConsumableType ConsumableType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable")
	float UseTime; // 사용에 걸리는 시간 (초)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable")
	float HealAmount; // 회복량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable")
	float MaxHealLimit; // 회복 한계선 (예: 75.0 또는 100.0)


	// 탄도학 관련 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	float MuzzleVelocity; // 탄속 (예: 900.0)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	float BulletGravityScale; // 낙차 (보통 1.0, 저격총은 더 낮게 가능)


	// 사격 관련 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float FireRate; // 연사 속도 (0.1이면 초당 10발)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float Damage;


	// 조준
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|ADS")
	float ADSFov; // 조준 시 FOV (낮을수록 줌인)


	// 연사
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	TArray<EFireMode> SupportedFireModes; // 해당 무기가 지원하는 모드들 


	// 반동 관련 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float VerticalRecoilAmount; // 사격 시 에임이 위로 튀는 기본값

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float HorizontalRecoilAmount; // 사격 시 에임이 좌우로 흔들리는 범위

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilModifier_ADS; // 조준(ADS) 중일 때 반동 배율 (보통 0.5~0.8)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	float HipFireSpread; // 비조준 사격 시 탄이 퍼지는 각도 (값이 클수록 많이 튐)

	// FItemData 구조체 내부에 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilRecoveryRate; // 복구 속도 (값이 클수록 빠르게 내려옴)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
	float RecoilRecoveryFraction; // 복구 비율 (0.5면 쏜 반동의 50%만 복구됨)


	// 장탄수, 장전

	// FItemData 구조체 내부에 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	FName AmmoItemID; // 이 총이 사용하는 탄약의 ID (예: "Ammo_556")

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
	int32 MaxAmmo; // 탄창 용량 (예: 30)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animations")
	UAnimMontage* ReloadMontage;

};

USTRUCT(BlueprintType)
struct FSlotData
{
	GENERATED_BODY()

public:
	FSlotData()
		: ItemID(NAME_None), Quantity(0), CurrentDurability(100.0f) {
	}

	// 데이터 테이블의 Row Name과 일치시킬 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	// 아이템 개수 (탄약이나 붕대 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity;

	// 내구도 (방탄복, 헬멧 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentDurability;

};