// Copyright 2024 Nic, Vlad, Alex


#include "AsymLobbyGameState.h"


void AAsymLobbyGameState::Multicast_UpdateCountdown_Implementation(const int32 Count) const 
{
	CountdownUpdateDelegate.Broadcast(Count);
}

void AAsymLobbyGameState::UpdateCountdown(const int32 Count) const
{
	if(HasAuthority())
	{
		Multicast_UpdateCountdown(Count);
	}
}
