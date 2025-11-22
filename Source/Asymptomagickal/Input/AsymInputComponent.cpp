// Copyright 2024 Nic Vlad, Alex


#include "AsymInputComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymInputComponent)

UAsymInputComponent::UAsymInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UAsymInputComponent::AddInputMappings(const UAsymInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem)
{
	check(InputConfig);
	check(InputSubsystem);

	// Here we can handle any custom logic to add something from the input config if required
}

void UAsymInputComponent::RemoveInputMappings(const UAsymInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem)
{
	check(InputConfig);
	check(InputSubsystem);

	// Here we can handle any custom logic to remove input mappings that have been added above
}

void UAsymInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (const uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
