// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AsymGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class ASYMPTOMAGICKAL_API UAsymGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void ReturnToMainMenu() override;

private:
	void ResetUserAndSessionState();
};
