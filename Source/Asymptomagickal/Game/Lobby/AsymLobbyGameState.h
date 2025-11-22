// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "Asymptomagickal/Game/AsymGameState.h"
#include "AsymLobbyGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCountdownUpdateDelegate, int32 Count);

/**
 * 
 */
UCLASS()
class ASYMPTOMAGICKAL_API AAsymLobbyGameState : public AAsymGameState
{
	     GENERATED_BODY()
public:
	void UpdateCountdown(const int32 Count) const;

	FCountdownUpdateDelegate CountdownUpdateDelegate;

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_UpdateCountdown(const int32 Count) const;
};
