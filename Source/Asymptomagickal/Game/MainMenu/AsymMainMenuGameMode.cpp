// Copyright 2024 Nic, Vlad, Alex


#include "AsymMainMenuGameMode.h"

#include "Asymptomagickal/Subsystem/AsymSessionSubsystem.h"
#include "Asymptomagickal/Subsystem/AsymUserSubsystem.h"
#include "Kismet/GameplayStatics.h"

void AAsymMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	bool bWasHardDisconnect = false;
	UGameInstance* GameInstance = GetGameInstance();

	if(UGameplayStatics::HasOption(OptionsString, TEXT("closed")))
	{
		bWasHardDisconnect = true;
	}

	// Only reset users on hard disconnect
	UAsymUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UAsymUserSubsystem>();
	if (ensure(UserSubsystem) && bWasHardDisconnect)
	{
		UserSubsystem->ResetUserState();
	}

	// Always reset sessions
	UAsymSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAsymSessionSubsystem>();
	if (ensure(SessionSubsystem))
	{
		SessionSubsystem->CleanUpSessions();
	}
}
