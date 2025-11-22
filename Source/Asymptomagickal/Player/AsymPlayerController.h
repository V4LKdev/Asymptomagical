// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AsymPlayerController.generated.h"

class UTileInteraction;
class UInputAction;
class UInputMappingContext;
class UAsymAbilitySystemComponent;

UCLASS()
class ASYMPTOMAGICKAL_API AAsymPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AAsymPlayerController();
	virtual void SetupInputComponent() override;
	
	/** Method called after processing input, we pass it on to a potential ability system component to handle ability input */
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> GenericIMC;

	UPROPERTY(BlueprintReadOnly, Category="Tile Interaction")
	TObjectPtr<UTileInteraction> TileManager;

private:
	
	TObjectPtr<UAsymAbilitySystemComponent> AsymAbilitySystemComponent;

	UAsymAbilitySystemComponent* GetASC();
};
