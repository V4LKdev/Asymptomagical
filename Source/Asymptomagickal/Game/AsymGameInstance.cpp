// Copyright 2024 Nic, Vlad, Alex


#include "AsymGameInstance.h"

#include "Asymptomagickal/Subsystem/AsymSessionSubsystem.h"
#include "Asymptomagickal/Subsystem/AsymUserSubsystem.h"

void UAsymGameInstance::ReturnToMainMenu()
{
	ResetUserAndSessionState();
	
	Super::ReturnToMainMenu();
}

void UAsymGameInstance::ResetUserAndSessionState()
{
	UAsymUserSubsystem* UserSubsystem = GetSubsystem<UAsymUserSubsystem>();
	if (ensure(UserSubsystem))
	{
		UserSubsystem->ResetUserState();
	}

	UAsymSessionSubsystem* SessionSubsystem = GetSubsystem<UAsymSessionSubsystem>();
	if (ensure(SessionSubsystem))
	{
		SessionSubsystem->CleanUpSessions();
	}
}
