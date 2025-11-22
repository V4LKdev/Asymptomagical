// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Components/WidgetComponent.h"
#include "AsymCharacter.generated.h"

struct FInputActionValue;
class UAsymInputConfig;
class UAsymAbilitySet;
class UAsymAttributeSet;
class UAsymAbilitySystemComponent;
class AAsymPlayerState;
class AAsymPlayerController;
class UInputMappingContext;

UCLASS()
class ASYMPTOMAGICKAL_API AAsymCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAsymCharacter();

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	UFUNCTION(BlueprintCallable, Category="AsymInstinct|Character")
	AAsymPlayerController* GetAsymPlayerController() const;

	UFUNCTION(BlueprintCallable, Category="AsymInstinct|Character")
	AAsymPlayerState* GetAsymPlayerState() const;

	UFUNCTION(BlueprintCallable, Category="AsymInstinct|Character")
	UAsymAbilitySystemComponent* GetAsymAbilitySystemComponent() const { return AbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, Category="AsymInstinct|Character")
	UAsymAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UFUNCTION(BlueprintImplementableEvent)
	void K2OnRepPlayerState();

	
protected:
	virtual void InitAbilityActorInfo();
	void AddCharacterAbilities();

	virtual bool CanJumpInternal_Implementation() const override;

	UFUNCTION()
	void Input_Move(const FInputActionValue& Value);
	UFUNCTION()
	void Input_Look_Mouse(const FInputActionValue& Value);
	UFUNCTION()
	void Input_Look_Stick(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputSystem|Input")
	TObjectPtr<UAsymInputConfig> InputConfig;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputSystem|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY()
	UAsymAbilitySystemComponent* AbilitySystemComponent;
	TObjectPtr<UAsymAttributeSet> AttributeSet;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InputSystem|Abilities")
	TObjectPtr<UAsymAbilitySet> AbilitySet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> OverheadWidget;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UUserWidget> PlayerTagWidgetClass;

	UPROPERTY(EditAnywhere)
	float LookSensitivity = 0.7f;

private:
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);

	void LoadTagWidget() const;
};
