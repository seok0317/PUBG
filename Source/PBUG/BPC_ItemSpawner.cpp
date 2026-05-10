#include "BPC_ItemSpawner.h"
#include "ItemSpawnPoint.h"
#include "ItemBase.h"
#include "Kismet/GameplayStatics.h"

UBPC_ItemSpawner::UBPC_ItemSpawner()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBPC_ItemSpawner::InitializeItemSpawning()
{
    // 서버에서만 실행
    if (GetOwnerRole() < ROLE_Authority) return;

    // 1. 맵의 모든 스폰 포인트를 찾음
    TArray<AActor*> SpawnPoints;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AItemSpawnPoint::StaticClass(), SpawnPoints);

    for (AActor* Point : SpawnPoints)
    {
        // 2. 스폰 확률 체크
        if (FMath::FRand() <= SpawnChance)
        {
            SpawnItemBundle(Point->GetActorLocation());
        }
    }
}

void UBPC_ItemSpawner::SpawnItemBundle(FVector Location)
{
    if (!ItemDataTable || !ItemBaseClass) return;

    FName SelectedID = GetRandomItemID();
    if (SelectedID.IsNone()) return;

    FItemData* Data = ItemDataTable->FindRow<FItemData>(SelectedID, TEXT(""));
    if (!Data) return;

    // 1. 바닥 찾기 (Line Trace)
    FVector Start = Location + FVector(0.0f, 0.0f, 100.0f); // 포인트의 1m 위에서
    FVector End = Location - FVector(0.0f, 0.0f, 100.0f);   // 포인트의 1m 아래로 레이저 발사

    FHitResult HitResult;
    FCollisionQueryParams Params;
    FVector FinalSpawnLoc = Location; // 기본값

    // 레이저가 무언가(바닥)에 맞았다면 그 위치를 스폰 지점으로!
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
    {
        FinalSpawnLoc = HitResult.Location + FVector(0.0f, 0.0f, 10.0f); // 바닥에서 5cm만 띄움
    }

    FActorSpawnParameters SpawnParams;
    // 겹쳐도 일단 소환하고 자리를 잡게 함
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 2. 메인 아이템 스폰
    AItemBase* MainItem = GetWorld()->SpawnActor<AItemBase>(
        ItemBaseClass,
        FinalSpawnLoc,
        FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f),
        SpawnParams
    );

    if (MainItem)
    {
        MainItem->ItemDataTable = ItemDataTable;
        MainItem->ItemID = SelectedID;
        int32 DefaultQuantity = 1; // 기본값은 1개

        if (Data->ItemType == EItemType::Ammo)
        {
            DefaultQuantity = 30; // 탄약은 30발
        }
        else if (Data->ItemType == EItemType::Consumable)
        {
            // 소모품 중에서 '붕대'인 경우에만 5개로 설정
            if (Data->ConsumableType == EConsumableType::Bandage)
            {
                DefaultQuantity = 5;
            }
            // 구상이나 의료용 키트 등은 기본값 1 유지
        }

        MainItem->Quantity = DefaultQuantity;
        MainItem->UpdateItemVisual();

        // 3. 총기 옆 탄약 세트 스폰
        if (Data->ItemType == EItemType::Weapon && !Data->AmmoItemID.IsNone())
        {
            FVector RightVec = MainItem->GetActorRightVector();
            FVector ForwardVec = MainItem->GetActorForwardVector();

            for (int32 i = 0; i < 2; i++)
            {
                // 탄약도 바닥 높이에 맞게 설정
                FVector AmmoLoc = FinalSpawnLoc + (RightVec * 25.0f) + (ForwardVec * (static_cast<float>(i) * 15.0f));

                AItemBase* AmmoItem = GetWorld()->SpawnActor<AItemBase>(
                    ItemBaseClass,
                    AmmoLoc,
                    MainItem->GetActorRotation(),
                    SpawnParams
                );

                if (AmmoItem)
                {
                    AmmoItem->ItemDataTable = ItemDataTable;
                    AmmoItem->ItemID = Data->AmmoItemID;
                    AmmoItem->Quantity = 30;
                    AmmoItem->UpdateItemVisual();
                }
            }
        }
    }
}

FName UBPC_ItemSpawner::GetRandomItemID()
{
    TArray<FName> AllRowNames = ItemDataTable->GetRowNames();
    if (AllRowNames.Num() == 0) return NAME_None;

    // 완전히 랜덤하게 하나 선택 (나중에 등급별 확률을 넣고 싶으면 여기서 수정)
    int32 RandomIndex = FMath::RandRange(0, AllRowNames.Num() - 1);
    return AllRowNames[RandomIndex];
}