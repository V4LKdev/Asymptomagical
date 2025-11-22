// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Asymptomagickal/Character/AsymCharacter.h"
#include "AsymPlayerState.generated.h"

class UAsymAttributeSet;
class AAsymPlayerController;
class UAsymAbilitySystemComponent;

/**
 * AAsymPlayerState
 *
 *	Base player state class used by this project.
 */
UCLASS(Config = Game)
class ASYMPTOMAGICKAL_API AAsymPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AAsymPlayerState();

	UFUNCTION(BlueprintCallable, Category="EsotericInstinct|PlayerState")
	AAsymPlayerController* GetAsymPlayerController() const;
	
	UFUNCTION(BlueprintCallable, Category="EsotericInstinct|PlayerState")
	UAsymAbilitySystemComponent* GetAsymAbilitySystemComponent() const { return AbilitySystemComponent; };
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UAsymAttributeSet* GetAttributeSet() const { return AttributeSet; }

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAsymAbilitySystemComponent> AbilitySystemComponent;
	UPROPERTY()
	TObjectPtr<UAsymAttributeSet> AttributeSet;
};
