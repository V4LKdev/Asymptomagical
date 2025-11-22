// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "AsymLobbyGameState.h"
#include "Asymptomagickal/Game/AsymGameMode.h"
#include "GameFramework/GameMode.h"
#include "AsymLobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ASYMPTOMAGICKAL_API AAsymLobbyGameMode : public AAsymGameMode
{
	GENERATED_BODY()
public:
	AAsymLobbyGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

	virtual void StartMatch() override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lobby")
	int32 MaxPlayers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lobby")
	float CountdownDuration;
	
private:
	UFUNCTION()
	void BeginCountdown();
	UFUNCTION()
	void UpdateCountdown();

	FTimerHandle CountdownHandle;
	FTimerDelegate UpdateCountdownDelegate;

	int32 CountdownTime;

	AAsymLobbyGameState* GetLobbyGameState() const { return Cast<AAsymLobbyGameState>(GameState); }
};
