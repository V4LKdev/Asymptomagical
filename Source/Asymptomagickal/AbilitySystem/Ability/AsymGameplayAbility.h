// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AsymGameplayAbility.generated.h"

/**
 * Defines how the ability is meant to be activated
 */
UENUM(BlueprintType)
enum class EAsymAbilityActivationPolicy : uint8
{
	AAP_OnInputTriggered UMETA(DisplayName = "On Input Triggered", Tooltip = "Activate the ability when the input is triggered."),
	AAP_WhileInputActive UMETA(DisplayName = "While Input Active", Tooltip = "Activate the ability continuously while the input is held active."),
	AAP_OnSpawn			 UMETA(DisplayName = "On Spawn", Tooltip = "Activate the ability when the Avatar is assigned")
};

/**
 * UAsymGameplayAbility
 *
 *	The base gameplay ability class used by this project.
 */
UCLASS()
class ASYMPTOMAGICKAL_API UAsymGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	friend class UAsymAbilitySystemComponent;
public:
	UAsymGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "AsymInstinct|Ability")
	UAsymAbilitySystemComponent* GetAsymAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "AsymInstinct|Ability")
	AAsymPlayerController* GetAsymPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "AsymInstinct|Ability")
	AController* GetControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "AsymInstinct|Ability")
	AAsymCharacter* GetAsymCharacterFromActorInfo() const;

	FORCEINLINE EAsymAbilityActivationPolicy GetActivationPolicy() const {return ActivationPolicy;}

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability Activation")
	EAsymAbilityActivationPolicy ActivationPolicy;
};
