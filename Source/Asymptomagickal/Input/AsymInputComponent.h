// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "AsymInputConfig.h"
#include "EnhancedInputComponent.h"
#include "AsymInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

/**
 * UAsymInputComponent
 *
 *	Component used to manage input mappings and bindings using an input config data asset.
 */
UCLASS(Config=Input)
class ASYMPTOMAGICKAL_API UAsymInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()
public:
	UAsymInputComponent(const FObjectInitializer& ObjectInitializer);

	static void AddInputMappings(const UAsymInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem);
	static void RemoveInputMappings(const UAsymInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem);
	void RemoveBinds(TArray<uint32>& BindHandles);

	template<class UserClass, typename FuncType>
	void BindNativeAction(const UAsymInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UAsymInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);
};


template <class UserClass, typename FuncType>
void UAsymInputComponent::BindNativeAction(const UAsymInputConfig* InputConfig, const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
{
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UAsymInputComponent::BindAbilityActions(const UAsymInputConfig* InputConfig, UserClass* Object,
	PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);
	for (const FAsymInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			}
		}
	}
}

