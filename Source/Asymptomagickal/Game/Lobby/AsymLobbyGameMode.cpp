// Copyright 2024 Nic, Vlad, Alex


#include "AsymLobbyGameMode.h"

#include "Asymptomagickal/Subsystem/AsymSessionSubsystem.h"
#include "GameFramework/GameStateBase.h"

AAsymLobbyGameMode::AAsymLobbyGameMode()
{
	GameStateClass = AAsymLobbyGameState::StaticClass();
	MaxPlayers = 3;
	CountdownDuration = 10.f;
	CountdownTime = CountdownDuration;
	bUseSeamlessTravel = true;
}

void AAsymLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const int32 Players = GetLobbyGameState()->PlayerArray.Num();

	if(Players == MaxPlayers)
	{
		BeginCountdown();
	}
}

void AAsymLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	const int32 Players = GetLobbyGameState()->PlayerArray.Num();

	if (Players < MaxPlayers)
	{
		GetWorldTimerManager().ClearTimer(CountdownHandle);
	}
}

void AAsymLobbyGameMode::StartMatch()
{
	Super::StartMatch();
}

void AAsymLobbyGameMode::BeginCountdown()
{
	if(GetWorldTimerManager().IsTimerActive(CountdownHandle))
	{
		return;
	}

	UpdateCountdownDelegate.BindUObject(this, &ThisClass::UpdateCountdown);
	
	CountdownTime = CountdownDuration;
	GetLobbyGameState()->UpdateCountdown(CountdownTime);

	GetWorldTimerManager().SetTimer(CountdownHandle, UpdateCountdownDelegate, 1.f, true);
}

void AAsymLobbyGameMode::UpdateCountdown()
{
	CountdownTime--;

	GetLobbyGameState()->UpdateCountdown(CountdownTime);

	if(CountdownTime <= 0.f)
	{
		GetWorldTimerManager().ClearTimer(CountdownHandle);
		
		//Start Match
	}
}
