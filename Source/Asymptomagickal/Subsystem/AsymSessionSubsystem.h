// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "EsotericSessionSubsystem.h"

#include "AsymSessionSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ASYMPTOMAGICKAL_API UAsymSessionSubsystem : public UEsotericSessionSubsystem
{
	GENERATED_BODY()
private:
	virtual UEsotericSession_SearchSessionRequest* CreateSearchSessionRequest() override;
};
