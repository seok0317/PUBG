// Copyright Epic Games, Inc. All Rights Reserved.

#include "PBUGCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "DrawDebugHelpers.h"
#include "ItemBase.h" 
#include "EnhancedInputSubsystems.h"
#include "BPC_Equipment.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h" // 카메라 컴포넌트 접근용

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// APBUGCharacter

APBUGCharacter::APBUGCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh(), TEXT("S_CameraEye"));
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// 인벤토리 컴포넌트 장착
	InventoryComponent = CreateDefaultSubobject<UBPC_Inventory>(TEXT("InventoryComponent"));

	// 장비 컴포넌트 장착
	EquipmentComponent = CreateDefaultSubobject<UBPC_Equipment>(TEXT("EquipmentComponent"));

	bReplicates = true;
}

void APBUGCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	GetWorldTimerManager().SetTimer(InteractTimerHandle, this, &APBUGCharacter::CheckInteractables, 0.1f, true);
}

//////////////////////////////////////////////////////////////////////////
// Input

void APBUGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APBUGCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APBUGCharacter::Look);

		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APBUGCharacter::Interact);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APBUGCharacter::StartAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APBUGCharacter::StopAim);

		if (EquipSlot1Action)
		{
			EnhancedInputComponent->BindAction(EquipSlot1Action, ETriggerEvent::Started, this, &APBUGCharacter::EquipSlot1);
		}
		if (EquipSlot2Action)
		{
			EnhancedInputComponent->BindAction(EquipSlot2Action, ETriggerEvent::Started, this, &APBUGCharacter::EquipSlot2);
		}
		if (HolsterAction)
		{
			EnhancedInputComponent->BindAction(HolsterAction, ETriggerEvent::Started, this, &APBUGCharacter::HolsterWeapon);
		}
		if (FireAction)
		{
			// Started: 버튼을 누르는 순간 1회 실행 (단발)
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APBUGCharacter::Fire);
			// Completed: 버튼 뗌 (사격 중지)
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &APBUGCharacter::StopFire);
		}
		if (SwitchFireModeAction)
		{
			// 사격 모드 전환 (단발/연발)
			EnhancedInputComponent->BindAction(SwitchFireModeAction, ETriggerEvent::Started, this, &APBUGCharacter::SwitchFireMode);
		}

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
	// 무기 스위칭 바인딩
	
}

void APBUGCharacter::Interact(const FInputActionValue& Value) {
	PickupItem(); // 아까 만든 줍기 함수 호출!
}

void APBUGCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APBUGCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

		// 플레이어가 마우스를 수직으로 움직이면 자동 복구 중단 (손맛 방해 금지)
		if (FMath::Abs(LookAxisVector.Y) > 0.0f && EquipmentComponent)
		{
			EquipmentComponent->ResetRecoilRecovery();
		}
	}
}

void APBUGCharacter::CheckInteractables()
{
	// 머리 소켓 위치 가져오기
	FVector Start = GetMesh()->GetSocketLocation(TEXT("spine_05"));
	FVector End = Start + (FollowCamera->GetForwardVector() * 500.0f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 이번 프레임에서 새로 찾은 액터를 담을 임시 변수
	AActor* CurrentFoundActor = nullptr;

	// 2. 레이저 쏘기
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		AActor* HitActor = HitResult.GetActor();

		// 인터페이스 확인
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
		{
			CurrentFoundActor = HitActor; // 아이템을 찾았음!
		}
	}
	// 3. 공용 변수 업데이트 (다른 곳에서 쓸 수 있게)
	TargetActor = CurrentFoundActor;

	// 4. [핵심] 이전 상태와 비교 (여기서 이벤트를 한 번만 발생시킴)
	if (CurrentFoundActor != LastTarget)
	{
		// 블루프린트에 신호를 보냄 (찾았으면 액터를, 못 찾았으면 nullptr을 보냄)
		OnTargetItemChanged(CurrentFoundActor);

		// 상태 업데이트
		LastTarget = CurrentFoundActor;
	}

}

void APBUGCharacter::PickupItem()
{
	if (TargetActor)
	{
		// 서버에게 줍기를 요청한다!
		Server_PickupItem(TargetActor);
	}
}

// 2. 서버 함수 (검증)
bool APBUGCharacter::Server_PickupItem_Validate(AActor* ItemToPickup)
{
	// 아이템이 null이 아니면 통과시키겠다는 뜻
	return ItemToPickup != nullptr;
}

// 3. 서버 함수 (실행)
void APBUGCharacter::Server_PickupItem_Implementation(AActor* ItemToPickup)
{
	// 1. 형변환 (Actor를 ItemBase로 바꿔서 ItemID에 접근)
	AItemBase* Item = Cast<AItemBase>(ItemToPickup);
	if (!Item || !Item->ItemDataTable) return;

	// 1. 데이터 테이블에서 타입 확인
	FItemData* Data = Item->ItemDataTable->FindRow<FItemData>(Item->ItemID, TEXT(""));

	if (Data)
	{
		// 2. 만약 무기라면? -> 장착 컴포넌트로 보냄
		if (Data->ItemType == EItemType::Weapon)
		{
			EquipmentComponent->Server_EquipWeapon(Item->ItemID);
			ItemToPickup->Destroy();
		}
		// 3. 소모품이나 탄약이라면? -> 기존 인벤토리로 보냄
		else
		{
			if (InventoryComponent->AddToInventory(Item->ItemID, Item->Quantity))
			{
				ItemToPickup->Destroy();
			}
		}
	}
}

void APBUGCharacter::EquipSlot1(const FInputActionValue& Value)
{
	// 1번 슬롯 꺼내기
	if (EquipmentComponent)
	{
		EquipmentComponent->Server_EquipWeaponIndex(1);
	}
}

void APBUGCharacter::EquipSlot2(const FInputActionValue& Value)
{
	// 2번 슬롯 꺼내기
	if (EquipmentComponent)
	{
		EquipmentComponent->Server_EquipWeaponIndex(2);
	}
}

void APBUGCharacter::HolsterWeapon(const FInputActionValue& Value)
{
	// 무기 집어넣기 (0번 슬롯 = 맨손)
	if (EquipmentComponent)
	{
		EquipmentComponent->Server_EquipWeaponIndex(0);
	}
}

void APBUGCharacter::Fire(const FInputActionValue& Value)
{
	if (EquipmentComponent)
	{
		EquipmentComponent->StartFire();
	}
}

// 사격 중지 함수 구현
void APBUGCharacter::StopFire(const FInputActionValue& Value)
{
	
	if (EquipmentComponent)
	{
		EquipmentComponent->StopFire();
	}
}

// 사격 모드 전환 함수 구현
void APBUGCharacter::SwitchFireMode(const FInputActionValue& Value)
{
	
	if (EquipmentComponent)
	{
		EquipmentComponent->Server_SwitchFireMode();
	}
}

bool APBUGCharacter::Server_DropItem_Validate(FName ItemID, int32 Quantity) { return true; }

void APBUGCharacter::Server_DropItem_Implementation(FName ItemID, int32 Quantity)
{
	if (InventoryComponent) InventoryComponent->DropItem(ItemID, Quantity);
}

bool APBUGCharacter::Server_DropWeapon_Validate(int32 SlotIndex) { return true; }

void APBUGCharacter::Server_DropWeapon_Implementation(int32 SlotIndex)
{
	if (EquipmentComponent) EquipmentComponent->DropWeapon(SlotIndex);
}

// 리플리케이션 등록
void APBUGCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APBUGCharacter, CurrentHealth);
	DOREPLIFETIME(APBUGCharacter, bIsDead);
}

float APBUGCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (GetLocalRole() < ROLE_Authority || CurrentHealth <= 0) return 0.f;

	// 체력 감소 및 제한
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

	UE_LOG(LogTemp, Warning, TEXT("Character Hit! Remaining Health: %f"), CurrentHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0) {
		bIsDead = true;
		OnRep_IsDead();
		Die();
	}
	return DamageAmount;
}

void APBUGCharacter::OnRep_CurrentHealth() {
	// 클라이언트에서 체력 값이 변경될 때마다 UI 업데이트 
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void APBUGCharacter::OnRep_IsDead()
{
	// 서버/클라이언트 공통으로 실행되는 외형 변화
	HandleDeathVisuals();
}

void APBUGCharacter::HandleDeathVisuals() // 클라이언트가 인식하는 함수 
{
	// 1. 래그돌 활성화
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

	// 2. 캡슐 컴포넌트 충돌 끄기 (안 그러면 래그돌이 캡슐에 걸려 이상하게 움직임)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APBUGCharacter::Die() { // 서버가 인식하는 함수
	// 사망 처리 (래그돌 등)
	if (GetLocalRole() == ROLE_Authority)
	{
		// 입력 중지 및 컨트롤러 해제
		DetachFromControllerPendingDestroy();

		// 무기 숨기기 (장착 중인 무기 액터가 있다면 파괴하거나 숨김)
		if (EquipmentComponent) {
			if (EquipmentComponent->MainWeapon1Actor) EquipmentComponent->MainWeapon1Actor->SetActorHiddenInGame(true);
			if (EquipmentComponent->MainWeapon2Actor) EquipmentComponent->MainWeapon2Actor->SetActorHiddenInGame(true);
		}
	}
}

void APBUGCharacter::StartAim(const FInputActionValue& Value) {
	if (EquipmentComponent) {
		EquipmentComponent->Server_SetAiming(true);
		K2_StartADSEffects();
	}
}

void APBUGCharacter::StopAim(const FInputActionValue& Value) {
	if (EquipmentComponent) {
		EquipmentComponent->Server_SetAiming(false);
		K2_StopADSEffects();
	}
}

// 에임 피치 계산 함수
float APBUGCharacter::GetAimPitch() const
{
	// 1. 카메라 각도와 캐릭터 몸 각도의 차이를 구함
	FRotator ControlRot = GetControlRotation();
	FRotator ActorRot = GetActorRotation();

	// 2. 두 각도의 차이 계산 후 정규화 (-180 ~ 180)
	FRotator Delta = ControlRot - ActorRot;
	Delta.Normalize();

	// 3. Pitch 값 반환 (배그 에임 오프셋은 보통 위가 +, 아래가 - 혹은 그 반대입니다)
	return Delta.Pitch;
}
