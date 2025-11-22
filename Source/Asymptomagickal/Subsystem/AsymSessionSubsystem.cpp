// Copyright 2024 Nic Vlad, Alex


#include "AsymSessionSubsystem.h"
#include "Asymptomagickal/AsymLogChannels.h"
#include "OnlineSubsystemUtils.h"

UEsotericSession_SearchSessionRequest* UAsymSessionSubsystem::CreateSearchSessionRequest()
{
	UEsotericSession_SearchSessionRequest* NewRequest = NewObject<UEsotericSession_SearchSessionRequest>(this);
	NewRequest->bUseLobbies = true;
	NewRequest->SearchString = FString("AsymLobby");
	
	UE_LOG(LogAsym, Log, TEXT("Created Default Search Session Request"));
	return NewRequest;
}
