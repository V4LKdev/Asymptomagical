// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AsymInputConfig.generated.h"

class UInputAction;
/**
 * FAsymInputAction
 *
 *	Struct used to map an input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct FAsymInputAction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(Categories="InputTag"))
	FGameplayTag InputTag = FGameplayTag();
};

/**
 * UAsymInputConfig
 *
 *	Non-mutable data asset that contains input configuration properties.
 */
UCLASS()
class ASYMPTOMAGICKAL_API UAsymInputConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UAsymInputConfig(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Asymptomagickal|Input")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag) const;
	
	UFUNCTION(BlueprintCallable, Category = "Asymptomagickal|Input")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag) const;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction", tooltip = "List of input actions used by the owner.  These input actions are mapped to a gameplay tag and must be manually bound."))
	TArray<FAsymInputAction> NativeInputActions;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction", tooltip = "List of input actions used by the owner.  These input actions are mapped to a gameplay tag and are automatically bound to abilities with matching input tags."))
	TArray<FAsymInputAction> AbilityInputActions;
};
