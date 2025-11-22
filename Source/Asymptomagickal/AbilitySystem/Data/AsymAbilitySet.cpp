// Copyright 2024 Nic Vlad, Alex


#include "AsymAbilitySet.h"

#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "Asymptomagickal/AsymLogChannels.h"
#include "Asymptomagickal/AbilitySystem/AsymAbilitySystemComponent.h"
#include "Asymptomagickal/AbilitySystem/Ability/AsymGameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymAbilitySet)

void FAsymAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if(Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FAsymAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FAsymAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* Set)
{
	GrantedAttributeSets.Add(Set);
}

void FAsymAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAsymAbilitySystemComponent* AsymASC)
{
	check(AsymASC);

	if (!AsymASC->IsOwnerActorAuthoritative())
	{
		// Must be authoritative to give or take ability sets.
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			AsymASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			AsymASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		AsymASC->RemoveSpawnedAttribute(Set);
	}

	AbilitySpecHandles.Reset();
	GameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

UAsymAbilitySet::UAsymAbilitySet(const FObjectInitializer& ObjectInitializer)
{
}

void UAsymAbilitySet::GiveToAbilitySystem(UAsymAbilitySystemComponent* InASC, FAsymAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(InASC);

	if(!InASC->IsOwnerActorAuthoritative())
	{
		// Must be authoritative to give or take ability sets.
		return; 
	}
	
	// Grant the gameplay abilities.
	for(int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FAsymAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];

		if(!IsValid(AbilityToGrant.Ability))
		{
			UE_LOG(LogAsymAbilitySystem, Error, TEXT("GrantedGameplayAbilities[%d] on ability set [%s] is not valid."), AbilityIndex, *GetNameSafe(this));
			continue;
		}

		UAsymGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UAsymGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO,  AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = InASC->GiveAbility(AbilitySpec);

		if(OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	// Grant the gameplay effects.
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FAsymAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogAsymAbilitySystem, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid"), EffectIndex, *GetNameSafe(this));
			continue;
		}

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle GameplayEffectHandle = InASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, InASC->MakeEffectContext());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}

	// Grant the attribute sets.
	for (int32 SetIndex = 0; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const FAsymAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];

		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOG(LogAsymAbilitySystem, Error, TEXT("GrantedAttributes[%d] on ability set [%s] is not valid"), SetIndex, *GetNameSafe(this));
			continue;
		}

		UAttributeSet* NewSet = NewObject<UAttributeSet>(InASC->GetOwner(), SetToGrant.AttributeSet);
		InASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}
}
