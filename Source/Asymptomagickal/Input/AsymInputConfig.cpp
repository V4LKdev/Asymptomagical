// Copyright 2024 Nic Vlad, Alex


#include "AsymInputConfig.h"

#include "Asymptomagickal/AsymLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymInputConfig)

UAsymInputConfig::UAsymInputConfig(const FObjectInitializer& ObjectInitializer)
{
}

const UInputAction* UAsymInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FAsymInputAction& Action : NativeInputActions)
	{
		if(Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}
	
	UE_LOG(LogAsym, Error, TEXT("Can't find NativeInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));

	return nullptr;
}

const UInputAction* UAsymInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FAsymInputAction& Action : AbilityInputActions)
	{
		if(Action.InputAction && Action.InputTag.MatchesTagExact(InputTag))
		{
			return Action.InputAction;
		}
	}
	
	UE_LOG(LogAsym, Error, TEXT("Can't find NativeInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));

	return nullptr;
}
