// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AItemBase::AItemBase()
{
	Quantity = 1;
	bReplicates = true;
	SetReplicateMovement(true); // 이동/위치 복제 활성화
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void AItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItemBase, Quantity); // 수량 복제 설정
	DOREPLIFETIME(AItemBase, ItemID);
	DOREPLIFETIME(AItemBase, ItemDataTable);
}

void AItemBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateItemVisual(); // 공용 함수 호출
}

void AItemBase::OnRep_ItemID()
{
	UpdateItemVisual(); // 데이터가 복제되면 외형 변경
}

void AItemBase::UpdateItemVisual()
{

	FString RoleString = (GetLocalRole() == ROLE_Authority) ? TEXT("Server") : TEXT("Client");
	UE_LOG(LogTemp, Warning, TEXT("[%s] UpdateItemVisual called for ID: %s"), *RoleString, *ItemID.ToString());

	if (ItemDataTable && !ItemID.IsNone())
	{
		FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));
		if (Data && MeshComponent)
		{
			MeshComponent->SetStaticMesh(Data->ItemMesh);
		}
	}
	else
	{
		// 데이터를 못 찾았을 때 (잘못된 이름 입력 시) 빈 메쉬로 초기화하여 크래시 방지
		MeshComponent->SetStaticMesh(nullptr);
	}
}

void AItemBase::GetInteractInfo_Implementation(FText& OutName, FText& OutAction)
{
	if (ItemDataTable && !ItemID.IsNone())
	{
		FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));
		if (Data)
		{
			OutName = Data->ItemName;
			OutAction = Data->ActionText;
			return;
		}
	}
	OutName = FText::FromString("Unknown Item");
	OutAction = FText::FromString("Pick Up");
}

// Called when the game starts or when spawned
void AItemBase::BeginPlay()
{
	Super::BeginPlay();
	
}





