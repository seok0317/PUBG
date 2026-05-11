// PBUGGameState.cpp

#include "PBUGGameState.h"
#include "Net/UnrealNetwork.h" // 리플리케이션을 위해 반드시 필요!

APBUGGameState::APBUGGameState()
{
}

void APBUGGameState::OnRep_AlivePlayerCount()
{
    // 값이 바뀌면 UI에게 방송
    OnAliveCountChanged.Broadcast(AlivePlayerCount);
}

void APBUGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 이 변수를 네트워크 복제하겠다고 등록
    DOREPLIFETIME(APBUGGameState, AlivePlayerCount);
}