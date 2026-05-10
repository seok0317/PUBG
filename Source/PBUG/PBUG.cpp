// PBUG.cpp
#include "PBUG.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystem.h"

class FPBUGModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        // 엔진 초기화 시점에 환경변수 강제 주입
        FPlatformMisc::SetEnvironmentVar(TEXT("SteamAppId"), TEXT("480")); 
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPBUGModule, PBUG, "PBUG");