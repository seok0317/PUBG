// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true; // 액터 복제 활성화

}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
}


void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeaponBase, ItemID); // ItemID 변수를 네트워크 복제 대상으로 등록
}

void AWeaponBase::OnRep_ItemID()
{
    // [핵심] 여기서 원천 봉쇄! ID가 None이면 블루프린트에게 시키지도 않음.
    if (!ItemID.IsNone())
    {
        // 유효한 ID일 때만 블루프린트의 비주얼 업데이트 실행
        K2_OnWeaponVisualUpdated();
    }
}

