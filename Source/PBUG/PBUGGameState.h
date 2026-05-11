// PBUGGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PBUGGameState.generated.h"

// 이 줄을 클래스 위에 추가하세요 (델리게이트 선언)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAliveCountChanged, int32, NewCount);

UCLASS()
class PBUG_API APBUGGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    APBUGGameState();

    // 남은 인원수 변수
    UPROPERTY(ReplicatedUsing = OnRep_AlivePlayerCount, BlueprintReadOnly, Category = "GameRules")
    int32 AlivePlayerCount = 0;

    UFUNCTION()
    void OnRep_AlivePlayerCount();

    UPROPERTY(BlueprintAssignable, Category = "GameRules|Events")
    FOnAliveCountChanged OnAliveCountChanged;

    // 리플리케이션을 위해 이 함수 선언이 꼭 필요합니다.
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};