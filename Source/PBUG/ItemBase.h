// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemData.h" 
#include "Engine/DataTable.h"
#include "InteractionInterface.h" 
#include "ItemBase.generated.h"

UCLASS()
class PBUG_API AItemBase : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemBase();

	// 에디터에서 행 이름을 고를 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemID, Category = "Item")
	FName ItemID;

	// 참조할 테이블 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	UDataTable* ItemDataTable;

	// 이 아이템 액터가 가지고 있는 수량 (기본값 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item")
	int32 ContainedAmmo = 0;

	UFUNCTION()
	void OnRep_ItemID();

	// 외형 업데이트를 위한 별도 함수 (OnConstruction과 공유)
	void UpdateItemVisual();

	// 리플리케이션을 위해 추가
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	// 에디터에서 배치를 바꿀 때마다 자동으로 모델링을 바꿔주는 함수
	virtual void OnConstruction(const FTransform& Transform) override;

	// 인터페이스 함수 구현
	virtual void GetInteractInfo_Implementation(FText& OutName, FText& OutAction) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	UStaticMeshComponent* MeshComponent;

 

};
