// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AsymWidgetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UAsymWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ASYMPTOMAGICKAL_API IAsymWidgetInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetText(const FText& NewText);
};
