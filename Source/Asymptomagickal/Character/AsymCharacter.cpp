// Copyright 2024 Nic Vlad, Alex


#include "AsymCharacter.h"

#include "Asymptomagickal/AsymGameplayTags.h"
#include "Asymptomagickal/AbilitySystem/AsymAbilitySystemComponent.h"
#include "Asymptomagickal/AbilitySystem/Data/AsymAbilitySet.h"
#include "Asymptomagickal/Input/AsymInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Asymptomagickal/AsymLogChannels.h"
#include "Asymptomagickal/AsymUtilities.h"
#include "Asymptomagickal/Interface/AsymWidgetInterface.h"
#include "Asymptomagickal/Player/AsymPlayerController.h"
#include "Asymptomagickal/Player/AsymPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsymCharacter)

namespace AsymPlayerCharacter
{
	static const float StickDriftCompensation = 0.2f;
	static const float LookYawRate = 150.0f;
	static const float LookPitchRate = 70.0f;
};

AAsymCharacter::AAsymCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SetNetCullDistanceSquared(900000000.0f);

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	//CapsuleComp->SetCollisionProfileName();

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Rotate mesh to be X forward since it is usually exported as Y forward.
	//MeshComp->SetCollisionProfileName();

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	BaseEyeHeight = 80.f;
	CrouchedEyeHeight = 50.f;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(MeshComp);
	OverheadWidget->SetWidgetSpace(EWidgetSpace::World);
}

void AAsymCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	
	Subsystem->AddMappingContext(DefaultMappingContext, 0);

	
	UAsymInputComponent* AsymInputComponent = Cast<UAsymInputComponent>(PlayerInputComponent);
	check(AsymInputComponent);

	TArray<uint32> BindHandles;
	
	AsymInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, BindHandles);

	AsymInputComponent->BindNativeAction(InputConfig, AsymGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	AsymInputComponent->BindNativeAction(InputConfig, AsymGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_Look_Mouse);
	AsymInputComponent->BindNativeAction(InputConfig, AsymGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_Look_Stick);
}

void AAsymCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D InputAxis = Value.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(ForwardDirection, InputAxis.Y);
	AddMovementInput(RightDirection, InputAxis.X);
}

void AAsymCharacter::Input_Look_Mouse(const FInputActionValue& Value)
{
	FVector2D LookValue = Value.Get<FVector2D>();

	if (LookValue == FVector2d(0.f, 0.f))
	{
		return;
	}
	
	AddControllerYawInput(LookValue.X * LookSensitivity);
	AddControllerPitchInput(LookValue.Y * LookSensitivity);
}

void AAsymCharacter::Input_Look_Stick(const FInputActionValue& Value)
{
	FVector2D LookValue = Value.Get<FVector2D>();

	if (LookValue.GetAbs().ComponentwiseAllLessThan(FVector2d(AsymPlayerCharacter::StickDriftCompensation, AsymPlayerCharacter::StickDriftCompensation)))
	{
		return;
	}

	const UWorld* World = GetWorld();
	check(World);

	AddControllerYawInput(LookValue.X * AsymPlayerCharacter::LookYawRate * World->GetDeltaSeconds() * LookSensitivity);
	AddControllerPitchInput(LookValue.Y * AsymPlayerCharacter::LookPitchRate * World->GetDeltaSeconds() * LookSensitivity);
}



void AAsymCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Init AbilityActorInfo on the Server
	InitAbilityActorInfo();
	AddCharacterAbilities();

	if(!IsRunningDedicatedServer())
	{
		LoadTagWidget();
	}
}

void AAsymCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Init AbilityActorInfo on the Client
	InitAbilityActorInfo();

	if(!HasAuthority())
	{
		K2OnRepPlayerState();

		LoadTagWidget();
	}

	//Redraw widget manually
	OverheadWidget->RequestRedraw();
}

void AAsymCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocallyControlled())
	{
		// Get the player's camera manager
		if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		{
			// Calculate the rotation to face the camera
			const FRotator NewRot = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CameraManager->GetCameraLocation());

			// Set the name tag's world rotation with yaw only
			OverheadWidget->SetWorldRotation(FRotator(0.f, NewRot.Yaw, 0.f));
		}
	}
}

void AAsymCharacter::AbilityInputTagPressed(FGameplayTag InputTag)
{
	AbilitySystemComponent->AbilityInputTagPressed(InputTag);
}

void AAsymCharacter::AbilityInputTagReleased(FGameplayTag InputTag)
{
	AbilitySystemComponent->AbilityInputTagReleased(InputTag);
}

void AAsymCharacter::LoadTagWidget() const
{
	if(IsLocallyControlled())
	{
		return;
	}
	
	checkf(PlayerTagWidgetClass, TEXT("No Valid Player Tag Widget Class Selected"));
		
	UUserWidget* PlayerTagWidget = CreateWidget(GetWorld(), PlayerTagWidgetClass);

	OverheadWidget->SetWidget(PlayerTagWidget);
	
	const FText PlayerNameText = FText::FromString(GetPlayerState()->GetPlayerName());
	
	IAsymWidgetInterface::Execute_SetText(PlayerTagWidget, PlayerNameText);
}

AAsymPlayerController* AAsymCharacter::GetAsymPlayerController() const
{
	return CastChecked<AAsymPlayerController>(Controller, ECastCheckedType::NullAllowed);
}

AAsymPlayerState* AAsymCharacter::GetAsymPlayerState() const
{
	return CastChecked<AAsymPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UAbilitySystemComponent* AAsymCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAsymCharacter::InitAbilityActorInfo()
{
	AAsymPlayerState* PS = GetPlayerState<AAsymPlayerState>();
	check(PS);

	AbilitySystemComponent = CastChecked<UAsymAbilitySystemComponent>(PS->GetAbilitySystemComponent());
	AbilitySystemComponent->InitAbilityActorInfo(PS, this);

	AttributeSet = PS->GetAttributeSet();
}

void AAsymCharacter::AddCharacterAbilities()
{
	if(HasAuthority() && IsValid(AbilitySystemComponent) && IsValid(AbilitySet))
	{
		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr, this);
	}
}

bool AAsymCharacter::CanJumpInternal_Implementation() const
{
	// same as ACharacter's implementation but without the crouch check
	return JumpIsAllowedInternal();
}



