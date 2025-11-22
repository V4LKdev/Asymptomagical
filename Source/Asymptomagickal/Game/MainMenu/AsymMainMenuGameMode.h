// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AsymMainMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ASYMPTOMAGICKAL_API AAsymMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
};
