#include "BPC_Inventory.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ItemBase.h" // AItemBase 포함
#include "InteractionInterface.h"
#include "Net/UnrealNetwork.h"

UBPC_Inventory::UBPC_Inventory()
{
	PrimaryComponentTick.bCanEverTick = false;

    // [추가] 이 컴포넌트가 네트워크를 통해 데이터를 주고받도록 설정
    SetIsReplicatedByDefault(true);
}

void UBPC_Inventory::BeginPlay()
{
	Super::BeginPlay();
}

void UBPC_Inventory::OnRep_InventoryArray()
{
    // 데이터가 진짜로 클라이언트에 도착했을 때만 방송을 쏩니다!
    OnInventoryUpdated.Broadcast();
}

bool UBPC_Inventory::AddToInventory(FName ID, int32 Amount)
{

    if (!ItemDataTable) return false;

    FItemData* Data = ItemDataTable->FindRow<FItemData>(ID, TEXT(""));
    if (!Data) return false;

    float WeightToAdd = Data->ItemWeight * Amount;
    if (CurrentWeight + WeightToAdd > MaxCapacity) return false;

    bool bChanged = false; // 데이터가 바뀌었는지 확인할 변수

    // 2. 중첩 로직
    if (Data->MaxStackSize > 1)
    {
        for (FSlotData& Slot : InventoryArray)
        {
            if (Slot.ItemID == ID && Slot.Quantity < Data->MaxStackSize)
            {
                Slot.Quantity += Amount;
                CurrentWeight += WeightToAdd;
                bChanged = true;
                break;
            }
        }
    }

    // 3. 새 슬롯 추가 (중첩되지 않았을 때)
    if (!bChanged)
    {
        FSlotData NewSlot;
        NewSlot.ItemID = ID;
        NewSlot.Quantity = Amount;
        InventoryArray.Add(NewSlot);
        CurrentWeight += WeightToAdd;
        bChanged = true;
    }

    // [핵심] 데이터가 바뀌었다면 서버에서 방송을 해줍니다.
    if (bChanged && GetOwnerRole() == ROLE_Authority)
    {
        OnInventoryUpdated.Broadcast();
    }

    return bChanged;
}

void UBPC_Inventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UBPC_Inventory, InventoryArray); // 배열을 동기화하겠다고 등록
    DOREPLIFETIME(UBPC_Inventory, CurrentWeight); // 추가
    DOREPLIFETIME(UBPC_Inventory, MaxCapacity);   // 추가
}

void UBPC_Inventory::DebugPrintInventory()
{
    
}

TArray<AActor*> UBPC_Inventory::GetVicinityItems(float Radius)
{
    TArray<AActor*> OutActors;
    AActor* Owner = GetOwner();
    if (!Owner) return OutActors;

    TArray<AActor*> OverlapActors;
    TArray<AActor*> IgnoreActors;
    IgnoreActors.Add(Owner);

    // 주변의 WorldDynamic 물체들을 감지
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        Owner->GetActorLocation(),
        Radius,
        ObjectTypes,
        nullptr,
        IgnoreActors,
        OverlapActors
    );

    // 인터페이스를 가진 액터만 필터링
    for (AActor* Actor : OverlapActors)
    {
        if (Actor && Actor->GetClass()->ImplementsInterface(UInteractionInterface::StaticClass()))
        {
            OutActors.Add(Actor);
        }
    }
    return OutActors;
}

void UBPC_Inventory::DropItem(FName ItemID, int32 Quantity)
{
    if (GetOwnerRole() < ROLE_Authority) return;

    // 1. 데이터 제거
    RemoveItemData(ItemID, Quantity);

    // 2. 바닥에 스폰
    SpawnItemOnGround(ItemID, Quantity);

    // 3. UI 갱신 방송
    OnInventoryUpdated.Broadcast();
}

void UBPC_Inventory::RemoveItemData(FName ItemID, int32 Quantity)
{

    for (int32 i = InventoryArray.Num() - 1; i >= 0; i--)
    {
        if (InventoryArray[i].ItemID == ItemID)
        {
            // 데이터 테이블에서 무게 정보 가져오기
            FItemData* Data = ItemDataTable->FindRow<FItemData>(ItemID, TEXT(""));
            float UnitWeight = Data ? Data->ItemWeight : 0.0f;

            if (InventoryArray[i].Quantity > Quantity)
            {
                // 뺴려는 수량보다 많으면 수량만 감소
                InventoryArray[i].Quantity -= Quantity;
                CurrentWeight -= (UnitWeight * Quantity);
                Quantity = 0; // 모두 제거함
            }
            else
            {
                // 수량이 딱 맞거나 부족하면 슬롯 통째로 삭제
                int32 RemovedQty = InventoryArray[i].Quantity;
                CurrentWeight -= (UnitWeight * RemovedQty);
                InventoryArray.RemoveAt(i);
                Quantity -= RemovedQty; // 남은 수량만큼 다음 슬롯에서 계속 찾음
            }
        }

        // 뺄 만큼 다 뺐으면 루프 종료
        if (Quantity <= 0) break;
    }

    // 데이터가 바뀌었으므로 UI 방송
    OnInventoryUpdated.Broadcast();
}

void UBPC_Inventory::SpawnItemOnGround(FName ItemID, int32 Quantity)
{
    if (ItemID.IsNone() || GetOwnerRole() < ROLE_Authority) return;

    AActor* Owner = GetOwner();

    // 1. 스폰할 대략적인 위치 계산 (캐릭터 앞 1.5미터)
    FVector StartLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 150.0f;
    FVector EndLoc = StartLoc - FVector(0.0f, 0.0f, 500.0f); // 아래로 5미터 레이저 발사

    // 2. 바닥 체크 (Line Trace)
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    FVector FinalSpawnLoc = StartLoc; // 바닥 못 찾을 경우 대비 기본값

    if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, Params))
    {
        // 바닥을 찾았다면, 부딪힌 지점에서 살짝 위(아이템 절반 높이 정도)를 스폰 지점으로 설정
        FinalSpawnLoc = HitResult.Location + FVector(0.0f, 0.0f, 10.0f);
    }

    // 3. 랜덤 회전 (배그처럼 아이템이 떨구는 방향마다 랜덤하게 돌아가게)
    FRotator SpawnRot = FRotator(0.0f, FMath::FRandRange(0.f, 360.f), 0.0f);

    // 4. 진짜 BP 클래스 소환 (ItemBaseClass 사용)
    UClass* ClassToSpawn = ItemBaseClass ? *ItemBaseClass : AItemBase::StaticClass();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AItemBase* NewItem = GetWorld()->SpawnActor<AItemBase>(ClassToSpawn, FinalSpawnLoc, SpawnRot, SpawnParams);

    if (NewItem)
    {
        NewItem->ItemDataTable = ItemDataTable;
        NewItem->Quantity = Quantity;
        NewItem->ItemID = ItemID;

        NewItem->UpdateItemVisual();

        UE_LOG(LogTemp, Warning, TEXT("Dropped Item: %s, Qty: %d"), *ItemID.ToString(), Quantity);
    }
}

int32 UBPC_Inventory::GetTotalQuantityByID(FName ItemID)
{
    int32 Total = 0;
    for (const FSlotData& Slot : InventoryArray)
    {
        if (Slot.ItemID == ItemID) Total += Slot.Quantity;
    }
    return Total;
}

void UBPC_Inventory::ConsumeItem(FName ItemID, int32 Quantity)
{
    RemoveItemData(ItemID, Quantity);
}