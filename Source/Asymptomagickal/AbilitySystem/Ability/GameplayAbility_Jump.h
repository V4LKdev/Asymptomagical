// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "AsymGameplayAbility.h"
#include "GameplayAbility_Jump.generated.h"

/**
 * UGameplayAbility_Jump
 *
 * Gameplay ability used for character jumping.
 */
UCLASS(Abstract)
class ASYMPTOMAGICKAL_API UGameplayAbility_Jump : public UAsymGameplayAbility
{
	GENERATED_BODY()
public:
	UGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Asym|Ability")
	void CharacterJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Asym|Ability")
	void CharacterJumpStop();
};
