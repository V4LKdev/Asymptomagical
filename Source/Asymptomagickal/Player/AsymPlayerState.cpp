// Copyright 2024 Nic Vlad, Alex


#include "AsymPlayerState.h"

#include "AbilitySystemComponent.h"
#include "AsymPlayerController.h"
#include "Asymptomagickal/AbilitySystem/AsymAbilitySystemComponent.h"
#include "Asymptomagickal/AbilitySystem/Attribute/AsymAttributeSet.h"
#include "Asymptomagickal/Character/AsymCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymPlayerState)

AAsymPlayerState::AAsymPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAsymAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed); // Mixed for Players, Minimal for Other

	// These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	AttributeSet = CreateDefaultSubobject<UAsymAttributeSet>("AttributeSet");

	// AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);
}

AAsymPlayerController* AAsymPlayerState::GetAsymPlayerController() const
{
	return Cast<AAsymPlayerController>(GetOwner());
}

UAbilitySystemComponent* AAsymPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
