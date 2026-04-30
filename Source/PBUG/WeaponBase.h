// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

UCLASS()
class PBUG_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UPROPERTY(ReplicatedUsing = OnRep_ItemID, EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName ItemID;

	// 네트워크 엔진이 부르는 C++ 내부 함수
	UFUNCTION()
	void OnRep_ItemID();

	// 2. 이건 로직을 통과했을 때 블루프린트에서 실제 외형을 바꾸는 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void K2_OnWeaponVisualUpdated();

	// 3. 멀티플레이 필수 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
