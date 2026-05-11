#include "BPC_Equipment.h"
#include "Net/UnrealNetwork.h"
#include "WeaponBase.h" 
#include "BulletProjectile.h" 
#include "GameFramework/ProjectileMovementComponent.h" // 컴포넌트 접근을 위해 필요
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h" 
#include "PBUGCharacter.h" 


#include "BPC_Inventory.h" 

UBPC_Equipment::UBPC_Equipment() 
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true); // 컴포넌트 복제 활성화
}



void UBPC_Equipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBPC_Equipment, MainWeapon1);
    DOREPLIFETIME(UBPC_Equipment, MainWeapon1Actor);

    // [추가] 2번 무기도 복제 등록
    DOREPLIFETIME(UBPC_Equipment, MainWeapon2);
    DOREPLIFETIME(UBPC_Equipment, MainWeapon2Actor);

	DOREPLIFETIME(UBPC_Equipment, BackpackSlot);

	// [추가] 현재 장착된 무기 인덱스도 복제 등록
	DOREPLIFETIME(UBPC_Equipment, CurrentWeaponIndex); // 추가!
	DOREPLIFETIME(UBPC_Equipment, bIsAiming);
	DOREPLIFETIME(UBPC_Equipment, CurrentFireMode);

	DOREPLIFETIME(UBPC_Equipment, AmmoInMag1);
	DOREPLIFETIME(UBPC_Equipment, AmmoInMag2);

	DOREPLIFETIME(UBPC_Equipment, bIsReloading);
}

bool UBPC_Equipment::Server_EquipWeapon_Validate(FName ItemID, int32 InAmmo) { return true; }

void UBPC_Equipment::Server_EquipWeapon_Implementation(FName ItemID, int32 InAmmo)
{


    // 1번 슬롯이 비어있으면 1번에 장착
    if (MainWeapon1.ItemID.IsNone())
    {
        MainWeapon1.ItemID = ItemID;
		AmmoInMag1 = InAmmo;
        MainWeapon1.Quantity = 1;
        SpawnWeaponVisual(1); // 1번 슬롯 스폰
    }
    // 1번이 차있고 2번 슬롯이 비어있으면 2번에 장착
    else if (MainWeapon2.ItemID.IsNone())
    {
        MainWeapon2.ItemID = ItemID;
		AmmoInMag2 = InAmmo;
        MainWeapon2.Quantity = 1;
        SpawnWeaponVisual(2); // 2번 슬롯 스폰
    }
    // 둘 다 차있으면? (임시: 1번 무기를 덮어씌움. 추후 버리기 로직 추가 필요)
    else
    {
		// [핵심] 현재 손에 들고 있는 슬롯 번호를 찾습니다. (맨손이면 1번 무기 선택)
		int32 SlotToReplace = (CurrentWeaponIndex > 0) ? CurrentWeaponIndex : 1;

		// [핵심] 기존 무기를 바닥에 버립니다. (이미 만든 DropWeapon 함수 활용)
		DropWeapon(SlotToReplace);

		// [핵심] 이제 슬롯이 비었으므로 새 무기를 넣습니다.
		if (SlotToReplace == 1)
		{
			MainWeapon1.ItemID = ItemID;
			MainWeapon1.Quantity = 1;
			SpawnWeaponVisual(1);
		}
		else
		{
			MainWeapon2.ItemID = ItemID;
			MainWeapon2.Quantity = 1;
			SpawnWeaponVisual(2);
		}

	}
	// 서버 측 UI 갱신 방송
	OnEquipmentUpdated.Broadcast();
}

void UBPC_Equipment::Server_EquipWeaponIndex_Implementation(int32 SlotIndex)
{
	// 1번 슬롯을 꺼내려는데, 1번에 총이 없다면 무시
	if (SlotIndex == 1 && MainWeapon1.ItemID.IsNone()) return;
	// 2번 슬롯을 꺼내려는데, 2번에 총이 없다면 무시
	if (SlotIndex == 2 && MainWeapon2.ItemID.IsNone()) return;

	// 현재 들고 있는 걸 또 누르면 (예: 1번 들고 있는데 1번 또 누름) -> 무기 집어넣기(0)
	if (CurrentWeaponIndex == SlotIndex)
	{
		CurrentWeaponIndex = 0;
	}
	else
	{
		// 그 외의 경우 해당 번호 무기 장착
		CurrentWeaponIndex = SlotIndex;
	}

	// 서버 본인의 화면(비주얼) 업데이트
	UpdateWeaponAttachment();
	OnAmmoUpdated.Broadcast();
}

bool UBPC_Equipment::Server_EquipWeaponIndex_Validate(int32 SlotIndex)
{
	return true; // 일단 무조건 통과시키겠다는 뜻
}


void UBPC_Equipment::OnRep_CurrentWeaponIndex()
{
	// 클라이언트들의 화면(비주얼) 업데이트
	UpdateWeaponAttachment();
	OnAmmoUpdated.Broadcast();
}

void UBPC_Equipment::OnRep_MainWeapon1()
{
    OnEquipmentUpdated.Broadcast(); // UI 갱신 신호 발송
}

void UBPC_Equipment::OnRep_MainWeapon2()
{
    OnEquipmentUpdated.Broadcast(); // UI 갱신 신호 발송
}

void UBPC_Equipment::UpdateWeaponAttachment()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner()); // 태그 관리를 위해 추가
	if (!OwnerCharacter || !PC) return;

	USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
	if (!CharacterMesh) return;

	// 1. 일단 모든 무기를 등(원래 위치)으로 보냅니다. (초기화)
	if (MainWeapon1Actor)
	{
		MainWeapon1Actor->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponBack_01"));
	}
	if (MainWeapon2Actor)
	{
		MainWeapon2Actor->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponBack_02"));
	}

	FGameplayTag ArmedTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Armed"));

	// 2. 현재 선택된 무기만 손으로 옮기고 태그 부착
	if (CurrentWeaponIndex > 0)
	{
		AActor* TargetActor = (CurrentWeaponIndex == 1) ? MainWeapon1Actor : MainWeapon2Actor;
		if (TargetActor)
		{
			TargetActor->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Weapon_R"));
			PC->AddGameplayTag(ArmedTag); // 무기 들었음 태그 추가
		}
	}
	else
	{
		PC->RemoveGameplayTag(ArmedTag); // 맨손이면 태그 제거
	}
}

void UBPC_Equipment::SpawnWeaponVisual(int32 SlotIndex)
{
	if (GetOwnerRole() < ROLE_Authority) return;

	// 타겟이 될 슬롯 데이터와 액터 포인터 셋업
	FSlotData* TargetSlotData = (SlotIndex == 1) ? &MainWeapon1 : &MainWeapon2;
	AActor** TargetActorPtr = (SlotIndex == 1) ? &MainWeapon1Actor : &MainWeapon2Actor;
	FName SocketName = (SlotIndex == 1) ? TEXT("WeaponBack_01") : TEXT("WeaponBack_02");

	// 1. 유효성 검사 (데이터 테이블이 없거나 아이템 ID가 없으면 기존 액터 삭제 후 중단)
	if (!ItemDataTable || TargetSlotData->ItemID.IsNone())
	{
		if (*TargetActorPtr)
		{
			(*TargetActorPtr)->Destroy();
			*TargetActorPtr = nullptr;
		}
		return;
	}

	// 2. 데이터 테이블 검색
	FItemData* Data = ItemDataTable->FindRow<FItemData>(TargetSlotData->ItemID, TEXT(""));
	if (!Data || !Data->WeaponClass) return;

	// 3. 기존 무기 삭제 (덮어쓰기 방지)
	if (*TargetActorPtr)
	{
		(*TargetActorPtr)->Destroy();
		*TargetActorPtr = nullptr;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 4. 액터 스폰 후 포인터 저장
	*TargetActorPtr = GetWorld()->SpawnActor<AActor>(
		Data->WeaponClass,
		OwnerCharacter->GetActorLocation(),
		OwnerCharacter->GetActorRotation(),
		SpawnParams
	);

	if (*TargetActorPtr)
	{
		AWeaponBase* Weapon = Cast<AWeaponBase>(*TargetActorPtr);
		if (Weapon)
		{
			Weapon->ItemID = TargetSlotData->ItemID;
			Weapon->OnRep_ItemID(); // 서버 비주얼 갱신
		}

		// 5. 1번이면 WeaponBack_01, 2번이면 WeaponBack_02 소켓에 부착
		(*TargetActorPtr)->AttachToComponent(
			OwnerCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketName
		);
	}
	UpdateWeaponAttachment();
}

bool UBPC_Equipment::Server_Fire_Validate(FVector MuzzleLocation, FRotator FireRotation)
{
	// 여기서 해킹 여부를 검사할 수 있습니다.
	// 예를 들어, 캐릭터와 총구 위치가 너무 멀면 거짓(false)을 반환해 강제 종료시킬 수 있습니다.
	// 지금은 테스트를 위해 무조건 true를 반환합니다.
	return true;
}

void UBPC_Equipment::Server_Fire_Implementation(FVector MuzzleLocation, FRotator FireRotation)
{
	// [디버그 1] 함수 호출 확인
	UE_LOG(LogTemp, Warning, TEXT("=== Fire Logic Started ==="));

	// [체크 1] 무기 장착 여부 및 데이터 테이블 확인
	if (CurrentWeaponIndex == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: No weapon equipped (CurrentWeaponIndex is 0)"));
		return;
	}
	if (!ItemDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: ItemDataTable is NULL! Check BPC_Equipment in Editor."));
		return;
	}

	// [체크 2] 아이템 ID 확인
	FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	if (CurrentID.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: Current Weapon ItemID is 'None'"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Current Weapon ID: %s"), *CurrentID.ToString());

	// [체크 3] 데이터 테이블 행 확인
	FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: Cannot find Row [%s] in DataTable"), *CurrentID.ToString());
		return;
	}

	// [체크 4] 탄속 확인 (0이면 제자리에 멈춰있음)
	if (Data->MuzzleVelocity <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Warning: MuzzleVelocity is 0. Bullet will not move!"));
	}

	if (CurrentWeaponIndex == 1 && AmmoInMag1 > 0) AmmoInMag1--;
	else if (CurrentWeaponIndex == 2 && AmmoInMag2 > 0) AmmoInMag2--;
	else return;

	OnAmmoUpdated.Broadcast();

	// [체크 5] 총알 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UClass* ClassToSpawn = BulletClass ? *BulletClass : ABulletProjectile::StaticClass();

	ABulletProjectile* Bullet = GetWorld()->SpawnActor<ABulletProjectile>(
		ClassToSpawn,
		MuzzleLocation,
		FireRotation,
		SpawnParams
	);

	if (Bullet)
	{
		UE_LOG(LogTemp, Log, TEXT("Bullet Spawned Successfully at: %s"), *MuzzleLocation.ToString());

		if (Bullet->ProjectileMovement)
		{

			Bullet->SetOwner(GetOwner()); // 총알의 주인을 사격한 캐릭터로 설정
			Bullet->SetInstigator(Cast<APawn>(GetOwner())); // 가해자 정보 설정

			// 데이터 주입
			Bullet->ProjectileMovement->InitialSpeed = Data->MuzzleVelocity;
			Bullet->ProjectileMovement->MaxSpeed = Data->MuzzleVelocity;
			Bullet->ProjectileMovement->ProjectileGravityScale = Data->BulletGravityScale;
			Bullet->DamageAmount = Data->Damage;

			// 벨로시티 직접 설정 (매우 중요)
			Bullet->ProjectileMovement->Velocity = FireRotation.Vector() * Data->MuzzleVelocity;

			UE_LOG(LogTemp, Log, TEXT("Bullet Velocity Set to: %f"), Data->MuzzleVelocity);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Bullet Spawned but ProjectileMovementComponent is NULL!"));
		}

		Multicast_PlayFireEffects();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: Actor Spawn Failed!"));
	}
}

void UBPC_Equipment::Multicast_PlayFireEffects_Implementation()
{
	// 클라이언트 본인의 총소리 및 총구 화염 파티클 재생
	// K2_PlayFireEffects(); // 블루프린트에서 사운드/파티클 처리 권장
}

void UBPC_Equipment::BeginPlay()
{
    Super::BeginPlay();

    // 필요한 초기화 로직이 있다면 여기에 작성합니다.
}

void UBPC_Equipment::DropWeapon(int32 SlotIndex)
{
	if (GetOwnerRole() < ROLE_Authority) return;

	FSlotData* TargetSlot = (SlotIndex == 1) ? &MainWeapon1 : &MainWeapon2;
	AActor** TargetActor = (SlotIndex == 1) ? &MainWeapon1Actor : &MainWeapon2Actor;
	int32* CurrentAmmo = (SlotIndex == 1) ? &AmmoInMag1 : &AmmoInMag2;

	if (TargetSlot->ItemID.IsNone()) return;

	// 1. 인벤토리 컴포넌트를 찾아 바닥에 아이템 스폰 요청
	auto* InvComp = GetOwner()->FindComponentByClass<UBPC_Inventory>();
	if (InvComp)
	{
		InvComp->SpawnItemOnGround(TargetSlot->ItemID, 1, *CurrentAmmo);
		
	}

	*CurrentAmmo = 0;

	// 2. 장착된 액터 파괴 및 데이터 초기화
	if (*TargetActor)
	{
		(*TargetActor)->Destroy();
		*TargetActor = nullptr;
	}
	*TargetSlot = FSlotData(); // 빈 데이터로 초기화

	// 3. 만약 버린 총이 현재 들고 있는 총이라면 맨손(0)으로 변경
	if (CurrentWeaponIndex == SlotIndex)
	{
		CurrentWeaponIndex = 0;
		OnRep_CurrentWeaponIndex(); // 비주얼 업데이트 호출
	}

	// 4. UI 갱신 방송
	OnEquipmentUpdated.Broadcast();
}

bool UBPC_Equipment::Server_SetAiming_Validate(bool bNewAiming) { return true; }

void UBPC_Equipment::Server_SetAiming_Implementation(bool bNewAiming) {
	if (bNewAiming && CurrentWeaponIndex == 0) return;

	bIsAiming = bNewAiming;
	// 서버(호스트) 본인의 비주얼 업데이트를 위해 호출

	// 조준중임을 태그로 관리
	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	if (PC)
	{
		FGameplayTag AimTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Aiming"));
		if (bNewAiming) PC->AddGameplayTag(AimTag);
		else PC->RemoveGameplayTag(AimTag);

		PC->RefreshMovementSpeed();
	}
	
	OnRep_IsAiming();
}

void UBPC_Equipment::OnRep_IsAiming() {
	// [이곳은 비워둡니다] 실제 카메라 이동은 캐릭터 블루프린트에서 이 변수를 보고 결정합니다.
}

float UBPC_Equipment::GetADSFOV() {
	// 현재 들고 있는 무기의 ID 가져오기
	FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	if (CurrentID.IsNone()) return 90.0f; // 맨손일 때 기본 FOV

	// 데이터 테이블에서 현재 무기의 ADS FOV 가져오기
	FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));
	return Data ? Data->ADSFov : 90.0f;
}



void UBPC_Equipment::GetADSHandTransform(FVector& OutLocation, FRotator& OutRotation)
{
	APBUGCharacter* Character = Cast<APBUGCharacter>(GetOwner());
	AActor* CurrentWeapon = (CurrentWeaponIndex == 1) ? MainWeapon1Actor : MainWeapon2Actor;

	if (!Character || !CurrentWeapon) return;

	USkeletalMeshComponent* CharMesh = Character->GetMesh();
	UStaticMeshComponent* WeaponMesh = CurrentWeapon->FindComponentByClass<UStaticMeshComponent>();

	if (CharMesh && WeaponMesh)
	{
		// 1. [핵심] 캐릭터 몸통 기준(Component Space)으로 가져옵니다.
		// 이렇게 하면 에임 오프셋으로 몸이 꺾여도, 몸통 기준의 상대적 거리는 유지됩니다.
		FVector EyeLoc = CharMesh->GetSocketLocation(TEXT("S_CameraEye"));
		EyeLoc = CharMesh->GetComponentTransform().InverseTransformPosition(EyeLoc);

		FVector HandLoc = CharMesh->GetSocketLocation(TEXT("hand_r"));
		HandLoc = CharMesh->GetComponentTransform().InverseTransformPosition(HandLoc);

		FVector SightLoc = WeaponMesh->GetSocketLocation(TEXT("S_AimPoint"));
		SightLoc = CharMesh->GetComponentTransform().InverseTransformPosition(SightLoc);

		// 2. 조준경에서 눈까지 가야 할 이동 벡터를 구합니다.
		FVector MoveVector = EyeLoc - SightLoc;


		// 4. 기존 손 위치에 이동 벡터를 더해줍니다 (컴포넌트 스페이스 결과값).
		OutLocation = HandLoc + MoveVector + ADSLocationOffset;

		// 5. 회전은 카메라 컨트롤 로테이션의 '캐릭터 상대 각도'를 줍니다.
		// (마우스를 내리면 총구도 정확히 그만큼 내려가게 함)
		FRotator AimRot;
		if (Character->IsLocallyControlled())
		{
			// 1. 내 화면이라면: 현재 컨트롤러의 실제 회전값을 즉시 사용 (반동 포함)
			AimRot = Character->GetControlRotation();
		}
		else
		{
			// 2. 다른 사람 화면이라면: 서버에서 복제되어 온 각도 사용
			AimRot = Character->GetRemoteAimRotation();
		}
		FRotator ActorRot = Character->GetActorRotation();

		FRotator RelativeAimRot = (AimRot - ActorRot).GetNormalized();

		OutRotation = RelativeAimRot + ADSRotationOffset;
	}
}

bool UBPC_Equipment::Server_SwitchFireMode_Validate() { return true; }

void UBPC_Equipment::Server_SwitchFireMode_Implementation()
{
	FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));

	if (Data && Data->SupportedFireModes.Num() > 1)
	{
		int32 CurrentIdx = Data->SupportedFireModes.Find(CurrentFireMode);
		int32 NextIdx = (CurrentIdx + 1) % Data->SupportedFireModes.Num();
		CurrentFireMode = Data->SupportedFireModes[NextIdx];

		OnEquipmentUpdated.Broadcast();
	}
	
}

// 사격 제어 로직 구현
void UBPC_Equipment::StartFire()
{
	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	if (!PC || PC->IsDead()) return;

	// 사격 시작 태그 부착 (Status이므로 Action 체크에 안 걸림)
	PC->AddGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Status.Firing")));
	
	// 즉시 한 발 발사
	FireTick();

	// 연사 모드일 경우에만 타이머 시작
	if (CurrentFireMode == EFireMode::FullAuto)
	{
		FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
		FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));

		if (Data && Data->FireRate > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(FireTimerHandle, this, &UBPC_Equipment::FireTick, Data->FireRate, true);
		}
		
	}
}

void UBPC_Equipment::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	if (PC) PC->RemoveGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Status.Firing")));
}

// 실제 발사 및 조준 보정 로직 (기존 캐릭터의 Fire 로직을 이식)
void UBPC_Equipment::FireTick()
{
	int32 CurrentAmmo, MaxAmmo;
	FName AmmoID;
	GetCurrentAmmoData(CurrentAmmo, MaxAmmo, AmmoID);

	APBUGCharacter* OwnerChar = Cast<APBUGCharacter>(GetOwner());

	FGameplayTagContainer ActionContainer;
	ActionContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Action")));

	if (!OwnerChar || OwnerChar->HasAnyMatchingGameplayTags(ActionContainer))
	{
		StopFire();
		return;
	}

	AActor* CurrentWeaponActor = (CurrentWeaponIndex == 1) ? MainWeapon1Actor : MainWeapon2Actor;
	if (!CurrentWeaponActor) return;

	// 탄약이 없으면 사격 중지
	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("탄약 없음! 재장전 필요"));
		StopFire();
		return;
	}

	UStaticMeshComponent* WeaponMesh = CurrentWeaponActor->FindComponentByClass<UStaticMeshComponent>();
	if (WeaponMesh)
	{
		// 1. 기본 조준 방향 (카메라 정중앙 기준)
		FVector CameraLoc = OwnerChar->GetFollowCamera()->GetComponentLocation();
		FVector CameraForward = OwnerChar->GetFollowCamera()->GetForwardVector();
		FVector TargetPoint = CameraLoc + (CameraForward * 10000.0f);
		FVector MuzzleLoc = WeaponMesh->GetSocketLocation(TEXT("Muzzle"));

		FRotator FireRot = (TargetPoint - MuzzleLoc).Rotation();

		// 2. [핵심] 비조준 시 탄 퍼짐(Spread) 적용
		if (!bIsAiming)
		{
			FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
			FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));
			if (Data)
			{
				// HipFireSpread 값만큼 무작위로 각도를 비틂
				float Spread = Data->HipFireSpread;
				FireRot.Pitch += FMath::FRandRange(-Spread, Spread);
				FireRot.Yaw += FMath::FRandRange(-Spread, Spread);
			}
		}

		// 3. 서버에 발사 요청 (조준 시엔 정확하게, 비조준 시엔 퍼진 각도로 전달)
		Server_Fire(MuzzleLoc, FireRot);

		// 4. 로컬 카메라 반동 적용
		ApplyRecoil();
	}
}

void UBPC_Equipment::ApplyRecoil()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	// 로컬 플레이어만 카메라가 움직여야 함
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));
	if (!Data) return;

	float FinalVertical = Data->VerticalRecoilAmount;
	float FinalHorizontal = Data->HorizontalRecoilAmount;

	if (bIsAiming)
	{
		// [조준 중] 화면 반동 강하게 (배그 정조준 사격 느낌)
		FinalVertical *= Data->RecoilModifier_ADS;
		FinalHorizontal *= Data->RecoilModifier_ADS;

		PC->AddPitchInput(-FinalVertical); // 위로 튐
		PC->AddYawInput(FMath::FRandRange(-FinalHorizontal, FinalHorizontal)); // 좌우 흔들림
	}
	else
	{
		// [비조준 중] 화면 반동은 최소화 (총알만 튀게 함)
		PC->AddPitchInput(-(FinalVertical * 0.1f));
		PC->AddYawInput(FMath::FRandRange(-(FinalHorizontal * 0.1f), (FinalHorizontal * 0.1f)));
	}

	APBUGCharacter* PCChar = Cast<APBUGCharacter>(OwnerPawn);
	if (PCChar)
	{
		PCChar->Server_SetAimRotation(PCChar->GetControlRotation());
	}

	// 2. [수정] 누적(+=)이 아니라 대입(=)으로 변경
	// 이렇게 하면 연사 중에는 계속 덮어씌워지다가 마지막 탄의 값만 남습니다.
	PendingRecoilRecovery = (FinalVertical * Data->RecoilRecoveryFraction);
}

void UBPC_Equipment::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 1. [추가] 현재 총을 쏘고 있는 중(타이머 작동 중)이라면 복구하지 않음
	if (GetWorld()->GetTimerManager().IsTimerActive(FireTimerHandle) || !bIsAiming)
	{
		return;
	}

	if (PendingRecoilRecovery > 0.0f)
	{
		APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
		if (PC)
		{
			FName CurrentID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
			FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentID, TEXT(""));

			if (Data)
			{
				// 2. [추가] 복구 속도를 조금 더 빠르게 설정 (마지막 탄이 툭 떨어지는 느낌)
				float RecoveryAmount = Data->RecoilRecoveryRate * DeltaTime;

				RecoveryAmount = FMath::Min(RecoveryAmount, PendingRecoilRecovery);
				PC->AddPitchInput(RecoveryAmount);
				PendingRecoilRecovery -= RecoveryAmount;
			}
		}
	}
}

bool UBPC_Equipment::Server_Reload_Validate() { return true; }

void UBPC_Equipment::Server_Reload_Implementation()
{
	if (CurrentWeaponIndex == 0) return;

	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	// 이미 무언가 행동 중이면 장전 불가
	if (!PC || PC->HasMatchingTag(FGameplayTag::RequestGameplayTag(FName("State.Action.Reloading")))) return;

	int32 CurrentAmmo, MaxAmmo;
	FName AmmoID;
	GetCurrentAmmoData(CurrentAmmo, MaxAmmo, AmmoID);

	// 이미 꽉 차있으면 재장전 불필요
	if (CurrentAmmo >= MaxAmmo) return;

	UBPC_Inventory* Inv = GetOwner()->FindComponentByClass<UBPC_Inventory>();
	if (!Inv) return;

	// 인벤토리에 해당 탄약이 있는지 확인
	int32 AvailableAmmo = Inv->GetTotalQuantityByID(AmmoID);
	if (AvailableAmmo <= 0) return;

	// 1. 재장전 시작 상태로 변경
	PC->AddGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Action.Reloading")));
	bIsReloading = true;

	// 2. 무기 데이터에서 몽타주 가져오기
	FName ItemID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));

	if (Data && Data->ReloadMontage)
	{
		// 3. 모든 클라이언트에게 애니메이션 재생 명령
		Multicast_PlayReloadAnim(Data->ReloadMontage);

		// 4. 애니메이션 길이만큼 기다린 후 탄약 채우기 (타이머)
		float AnimDuration = Data->ReloadMontage->GetPlayLength();
		GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UBPC_Equipment::FinishReloading, AnimDuration, false);
	}
	else
	{
		// 애니메이션이 없으면 즉시 완료
		FinishReloading();
	}
}

void UBPC_Equipment::Multicast_PlayReloadAnim_Implementation(UAnimMontage* MontageToPlay)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar)
	{
		OwnerChar->PlayAnimMontage(MontageToPlay);
	}
}

// 탄약이 실제로 차는 시점
void UBPC_Equipment::FinishReloading()
{
	if (!bIsReloading) return;

	APBUGCharacter* PC = Cast<APBUGCharacter>(GetOwner());
	if (PC) PC->RemoveGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Action.Reloading")));

	int32 CurrentAmmo, MaxAmmo;
	FName AmmoID;
	GetCurrentAmmoData(CurrentAmmo, MaxAmmo, AmmoID);

	UBPC_Inventory* Inv = GetOwner()->FindComponentByClass<UBPC_Inventory>();
	if (Inv)
	{
		int32 AmmoNeeded = MaxAmmo - CurrentAmmo;
		int32 AvailableAmmo = Inv->GetTotalQuantityByID(AmmoID);
		int32 AmmoToReload = FMath::Min(AmmoNeeded, AvailableAmmo);

		Inv->ConsumeItem(AmmoID, AmmoToReload);

		if (CurrentWeaponIndex == 1) AmmoInMag1 += AmmoToReload;
		else if (CurrentWeaponIndex == 2) AmmoInMag2 += AmmoToReload;
	}
	bIsReloading = false;
	OnAmmoUpdated.Broadcast(); // UI 갱신
}


// 데이터 테이블에서 정보를 가져오는 헬퍼 함수
void UBPC_Equipment::GetCurrentAmmoData(int32& OutCurrentAmmo, int32& OutMaxAmmo, FName& OutAmmoID)
{
	OutCurrentAmmo = (CurrentWeaponIndex == 1) ? AmmoInMag1 : AmmoInMag2;

	FName ItemID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));

	if (Data)
	{
		OutMaxAmmo = Data->MaxAmmo;
		OutAmmoID = Data->AmmoItemID;
	}
}

void UBPC_Equipment::OnRep_AmmoChanged()
{
	// 탄수가 바뀌면 UI에 알림
	OnAmmoUpdated.Broadcast();
}

void UBPC_Equipment::GetCurrentWeaponAmmoInfo(int32& CurrentMag, int32& TotalExtra)
{
	CurrentMag = 0;
	TotalExtra = 0;

	if (CurrentWeaponIndex == 0) return;

	// 1. 현재 탄창의 수량
	CurrentMag = (CurrentWeaponIndex == 1) ? AmmoInMag1 : AmmoInMag2;

	// 2. 예비 탄수 계산 (인벤토리에서 해당 탄약 ID 검색)
	FName CurrentItemID = (CurrentWeaponIndex == 1) ? MainWeapon1.ItemID : MainWeapon2.ItemID;
	FItemData* Data = ItemDataTable->FindRow<FItemData>(CurrentItemID, TEXT(""));

	if (Data)
	{
		UBPC_Inventory* Inv = GetOwner()->FindComponentByClass<UBPC_Inventory>();
		if (Inv)
		{
			TotalExtra = Inv->GetTotalQuantityByID(Data->AmmoItemID);
		}
	}
}

void UBPC_Equipment::OnRep_BackpackSlot()
{
	OnEquipmentUpdated.Broadcast(); // UI 갱신
}

bool UBPC_Equipment::Server_EquipBackpack_Validate(FName ItemID) { return true; }

void UBPC_Equipment::Server_EquipBackpack_Implementation(FName ItemID)
{
	if (!ItemDataTable) return;

	FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));
	if (!Data) return;

	// 1. 이미 가방을 메고 있다면? (배그는 더 좋은 가방으로 교체하거나 바닥에 떨굼)
	if (!BackpackSlot.ItemID.IsNone())
	{
		// 기존 가방을 바닥에 스폰하는 로직 (기존 DropWeapon과 유사하게 구현 가능)
		auto* InvComp = GetOwner()->FindComponentByClass<UBPC_Inventory>();
		if (InvComp) InvComp->SpawnItemOnGround(BackpackSlot.ItemID, 1);
	}

	// 2. 가방 정보 업데이트
	BackpackSlot.ItemID = ItemID;
	BackpackSlot.Quantity = 1;

	// 3. 인벤토리 컴포넌트의 용량 업데이트 (아까 만든 2단계 함수 호출)
	UBPC_Inventory* Inv = GetOwner()->FindComponentByClass<UBPC_Inventory>();
	if (Inv)
	{
		Inv->UpdateMaxCapacity(Data->CapacityBonus);
	}

	// 4. 가방 비주얼 스폰 (총 소환 로직과 거의 동일)
	// SpawnBackpackVisual(Data->ItemMesh); // 이 함수는 아래에서 설명

	OnEquipmentUpdated.Broadcast();
}