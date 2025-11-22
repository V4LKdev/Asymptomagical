// Copyright 2024 Nic Vlad, Alex


#include "AsymGameplayAbility.h"

#include "Asymptomagickal/AbilitySystem/AsymAbilitySystemComponent.h"
#include "Asymptomagickal/Character/AsymCharacter.h"
#include "Asymptomagickal/Player/AsymPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymGameplayAbility)

UAsymGameplayAbility::UAsymGameplayAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EAsymAbilityActivationPolicy::AAP_OnInputTriggered;
}

UAsymAbilitySystemComponent* UAsymGameplayAbility::GetAsymAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UAsymAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AAsymPlayerController* UAsymGameplayAbility::GetAsymPlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AAsymPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

AController* UAsymGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (const APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;
}

AAsymCharacter* UAsymGameplayAbility::GetAsymCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AAsymCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}
