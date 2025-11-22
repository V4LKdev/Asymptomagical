// Copyright 2024 Nic Vlad, Alex


#include "AsymPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Asymptomagickal/AbilitySystem/AsymAbilitySystemComponent.h"
#include "Asymptomagickal/Component/TileInteraction.h"
#include "Asymptomagickal/Input/AsymInputComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymPlayerController)

AAsymPlayerController::AAsymPlayerController()
{
	bReplicates = true;

	TileManager = CreateDefaultSubobject<UTileInteraction>(TEXT("TileInteractionComponent"));
	TileManager->SetIsReplicated(true);
}

void AAsymPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UAsymAbilitySystemComponent* ASC = GetASC())
	{
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AAsymPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if(ensure(InputSubsystem) && GenericIMC != nullptr)
	{
		InputSubsystem->AddMappingContext(GenericIMC, 1);
	}

	//UAsymInputComponent* Input = Cast<UAsymInputComponent>(InputComponent);
	//if(ensure(Input))
	//{
	//	Input->BindAction
	//}
}

void AAsymPlayerController::BeginPlay()
{
	Super::BeginPlay();
	const FInputModeGameOnly InputMode;
	
	SetInputMode(InputMode);
	SetShowMouseCursor(false);
}


UAsymAbilitySystemComponent* AAsymPlayerController::GetASC()
{
	if(AsymAbilitySystemComponent == nullptr)
	{
		AsymAbilitySystemComponent = Cast<UAsymAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return AsymAbilitySystemComponent;
}
