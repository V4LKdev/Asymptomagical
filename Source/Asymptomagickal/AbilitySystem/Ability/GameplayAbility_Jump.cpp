// Copyright 2024 Nic Vlad, Alex


#include "GameplayAbility_Jump.h"

#include "Asymptomagickal/Character/AsymCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayAbility_Jump)

struct FGameplayTagContainer;

UGameplayAbility_Jump::UGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool UGameplayAbility_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	const AAsymCharacter* AsymCharacter = Cast<AAsymCharacter>(ActorInfo->AvatarActor.Get());
	if (!AsymCharacter || !AsymCharacter->CanJump())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UGameplayAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Stop jumping in case the ability blueprint doesn't call it.
	CharacterJumpStop();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGameplayAbility_Jump::CharacterJumpStart()
{
	if (AAsymCharacter* AsymCharacter = GetAsymCharacterFromActorInfo())
	{
		if (AsymCharacter->IsLocallyControlled() && !AsymCharacter->bPressedJump)
		{
			AsymCharacter->UnCrouch();
			AsymCharacter->Jump();
		}
	}
}

void UGameplayAbility_Jump::CharacterJumpStop()
{
	if (AAsymCharacter* LyraCharacter = GetAsymCharacterFromActorInfo())
	{
		if (LyraCharacter->IsLocallyControlled() && LyraCharacter->bPressedJump)
		{
			LyraCharacter->StopJumping();
		}
	}
}
