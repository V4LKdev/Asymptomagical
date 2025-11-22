// Fill out your copyright notice in the Description page of Project Settings.


#include "EsotericUser/Public/EsotericSessionSubsystem.h"

#include "AssetRegistry/AssetData.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Engine/World.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/InputDeviceLibrary.h"
#include "Online/OnlineSessionNames.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EsotericSessionSubsystem)

DECLARE_LOG_CATEGORY_EXTERN(LogEsotericSession, Log, All);
DEFINE_LOG_CATEGORY(LogEsotericSession);

#define LOCTEXT_NAMESPACE "EsotericUser"

#pragma region OnlineSearchSettings

class FEsotericOnlineSearchSettingsBase : public FGCObject
{
public:
	FEsotericOnlineSearchSettingsBase(UEsotericSession_SearchSessionRequest* InSearchRequest)
	{
		SearchRequest = InSearchRequest;
	}

	virtual ~FEsotericOnlineSearchSettingsBase() override {}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(SearchRequest);
	}

	virtual FString GetReferencerName() const override
	{
		static const FString NameString = TEXT("FEsotericOnlineSearchSettings");
		return NameString;
	}

public:
	TObjectPtr<UEsotericSession_SearchSessionRequest> SearchRequest = nullptr;
};

class FEsotericSession_OnlineSessionSettings : public FOnlineSessionSettings
{
public:
	FEsotericSession_OnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4)
	{
		NumPublicConnections = MaxNumPlayers;
		if (NumPublicConnections < 0)
		{
			NumPublicConnections = 0;
		}
		NumPrivateConnections = 0;
		bIsLANMatch = bIsLAN;
		bShouldAdvertise = true;
		bAllowJoinInProgress = true;
		bAllowInvites = true;
		bUsesPresence = bIsPresence;
		bAllowJoinViaPresence = true;
		bAllowJoinViaPresenceFriendsOnly = false;
	}

	virtual ~FEsotericSession_OnlineSessionSettings() override {}
};

class FEsotericOnlineSearchSettings : public FOnlineSessionSearch, public FEsotericOnlineSearchSettingsBase
{
public:
	FEsotericOnlineSearchSettings(UEsotericSession_SearchSessionRequest* InSearchRequest) : FEsotericOnlineSearchSettingsBase(InSearchRequest)
	{
		bIsLanQuery = (InSearchRequest->OnlineMode == EEsotericSessionOnlineMode::LAN);
		MaxSearchResults = 9999;
		PingBucketSize = 50;
		
		if (InSearchRequest->bUseLobbies)
		{
			QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
			QuerySettings.Set(LOBBY_NAME_KEY, InSearchRequest->SearchString.ToLower(),  EOnlineComparisonOp::Equals);
			//QuerySettings.Set(SEARCH_LOBBIES, false, EOnlineComparisonOp::Equals); // Lyra has this set to true for some reason
		}
	}

	virtual ~FEsotericOnlineSearchSettings() override {}
};

#pragma endregion //Online Search Settings

#pragma region HostSessionRequest

int32 UEsotericSession_HostSessionRequest::GetMaxPlayers() const
{
	return MaxPlayerCount;
}

FString UEsotericSession_HostSessionRequest::GetMapName() const
{
	FAssetData MapAssetData;
	if (UAssetManager::Get().GetPrimaryAssetData(MapID, /*out*/ MapAssetData))
	{
		return MapAssetData.PackageName.ToString();
	}
	else
	{
		return FString();
	}
}

FString UEsotericSession_HostSessionRequest::ConstructTravelURL() const
{
	FString CombinedExtraArgs;

	if (OnlineMode == EEsotericSessionOnlineMode::LAN)
	{
		CombinedExtraArgs += TEXT("?bIsLanMatch");
	}
	
	if (OnlineMode != EEsotericSessionOnlineMode::Offline)
	{
		CombinedExtraArgs += TEXT("?listen");
	}

	for (const auto& KVP : ExtraArgs)
	{
		if (!KVP.Key.IsEmpty())
		{
			if (KVP.Value.IsEmpty())
			{
				CombinedExtraArgs += FString::Printf(TEXT("?%s"), *KVP.Key);
			}
			else
			{
				CombinedExtraArgs += FString::Printf(TEXT("?%s=%s"), *KVP.Key, *KVP.Value);
			}
		}
	}

	return FString::Printf(TEXT("%s%s"),
		*GetMapName(),
		*CombinedExtraArgs);
}

bool UEsotericSession_HostSessionRequest::ValidateAndLogErrors(FText& OutError) const
{
#if WITH_SERVER_CODE
	if (GetMapName().IsEmpty())
	{
		OutError = FText::Format(NSLOCTEXT("NetworkErrors", "InvalidMapFormat", "Can't find asset data for MapID {0}, hosting request failed."), FText::FromString(MapID.ToString()));
		return false;
	}

	return true;
#else
	// Client builds are only meant to connect to dedicated servers; they are missing the code to host a session by default.
	// You can change this behavior in subclasses to handle something like a tutorial
	OutError = NSLOCTEXT("NetworkErrors", "ClientBuildCannotHost", "Client builds cannot host game sessions.");
	return false;
#endif
}

#pragma endregion //HostSessionRequest

#pragma region SearchSession

FString UEsotericSession_SearchResult::GetDescription() const
{
	return Result.GetSessionIdStr();
}

void UEsotericSession_SearchResult::GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const
{
	bFoundValue = Result.Session.SessionSettings.Get<FString>(Key, /*out*/ Value);
}

void UEsotericSession_SearchResult::GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const
{
	bFoundValue = Result.Session.SessionSettings.Get<int32>(Key, /*out*/ Value);
}

int32 UEsotericSession_SearchResult::GetNumOpenPrivateConnections() const
{
	return Result.Session.NumOpenPrivateConnections;
}

int32 UEsotericSession_SearchResult::GetNumOpenPublicConnections() const
{
	return Result.Session.NumOpenPublicConnections;
}

int32 UEsotericSession_SearchResult::GetMaxPublicConnections() const
{
	return Result.Session.SessionSettings.NumPublicConnections;
}

int32 UEsotericSession_SearchResult::GetPingInMs() const
{
	return Result.PingInMs;
}

void UEsotericSession_SearchSessionRequest::NotifySearchFinished(bool bSucceeded, const FText& ErrorMessage) const
{
	OnSearchFinished.Broadcast(bSucceeded, ErrorMessage);
	K2_OnSearchFinished.Broadcast(bSucceeded, ErrorMessage);
}

#pragma endregion //Search Session


void UEsotericSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BindOnlineDelegates();
	GEngine->OnTravelFailure().AddUObject(this, &ThisClass::TravelLocalSessionFailure);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);

	UGameInstance* GameInstance = GetGameInstance();
	bIsDedicatedServer = GameInstance->IsDedicatedServerInstance();
}

void UEsotericSessionSubsystem::BindOnlineDelegates()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete));
	SessionInterface->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete));
	SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnUpdateSessionComplete));
	SessionInterface->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnEndSessionComplete));
	SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete));
	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete));
	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete));
	SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleSessionUserInviteAccepted));
	SessionInterface->AddOnSessionFailureDelegate_Handle(FOnSessionFailureDelegate::CreateUObject(this, &ThisClass::HandleSessionFailure));
}

void UEsotericSessionSubsystem::Deinitialize()
{
	if (const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld()))
	{
		// During shutdown, this may not be valid
		if (const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface())
		{
			SessionInterface->ClearOnSessionFailureDelegates(this);
		}
	}

	if (GEngine)
	{
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

bool UEsotericSessionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass
	return ChildClasses.Num() == 0;
}

void UEsotericSessionSubsystem::NotifySessionInformationUpdated(EEsotericSessionInformationState SessionStatusStr, const FString& GameMode, const FString& MapName) const
{
	OnSessionInformationChangedEvent.Broadcast(SessionStatusStr, GameMode, MapName);
	K2_OnSessionInformationChangedEvent.Broadcast(SessionStatusStr, GameMode, MapName);
}


UEsotericSession_HostSessionRequest* UEsotericSessionSubsystem::CreateHostSessionRequest()
{
	/** Game-specific subsystems can override this or you can modify after creation */
	UEsotericSession_HostSessionRequest* NewRequest = NewObject<UEsotericSession_HostSessionRequest>(this);
	NewRequest->bUseLobbies = true;
	
	UE_LOG(LogEsotericSession, Log, TEXT("Created Host Session Request"));
	return NewRequest;
}

UEsotericSession_SearchSessionRequest* UEsotericSessionSubsystem::CreateSearchSessionRequest()
{
	/** Game-specific subsystems can override this or you can modify after creation */
	UEsotericSession_SearchSessionRequest* NewRequest = NewObject<UEsotericSession_SearchSessionRequest>(this);
	NewRequest->bUseLobbies = true;
	
	UE_LOG(LogEsotericSession, Log, TEXT("Created Search Session Request"));
	return NewRequest;
}


#pragma region QuickPlay

void UEsotericSessionSubsystem::QuickPlaySession(APlayerController* JoiningOrHostingPlayer, UEsotericSession_HostSessionRequest* HostRequest)
{
	UE_LOG(LogEsotericSession, Log, TEXT("QuickPlay Requested"));

	if (HostRequest == nullptr)
	{
		UE_LOG(LogEsotericSession, Error, TEXT("QuickPlaySession passed a null request"));
		return;
	}

	TStrongObjectPtr<UEsotericSession_HostSessionRequest> HostRequestPtr = TStrongObjectPtr<UEsotericSession_HostSessionRequest>(HostRequest);
	TWeakObjectPtr<APlayerController> JoiningOrHostingPlayerPtr = TWeakObjectPtr<APlayerController>(JoiningOrHostingPlayer);

	UEsotericSession_SearchSessionRequest* QuickPlayRequest = CreateSearchSessionRequest();
	QuickPlayRequest->OnSearchFinished.AddUObject(this, &UEsotericSessionSubsystem::HandleQuickPlaySearchFinished, JoiningOrHostingPlayerPtr, HostRequestPtr);

	HostRequestPtr->bUseLobbies = true;
	QuickPlayRequest->bUseLobbies = true;

	NotifySessionInformationUpdated(EEsotericSessionInformationState::Matchmaking);
	FindSessionsInternal(JoiningOrHostingPlayer, CreateQuickPlaySearchSettings(HostRequest, QuickPlayRequest));
}

TSharedRef<FEsotericOnlineSearchSettings> UEsotericSessionSubsystem::CreateQuickPlaySearchSettings(UEsotericSession_HostSessionRequest* HostRequest, UEsotericSession_SearchSessionRequest* SearchRequest)
{
	TSharedRef<FEsotericOnlineSearchSettings> QuickPlaySearch = MakeShared<FEsotericOnlineSearchSettings>(SearchRequest);

	/** By default quick play does not want to include the map or game mode, games can fill this in as desired
	if (!HostRequest->ModeNameForAdvertisement.IsEmpty())
	{
		QuickPlaySearch->QuerySettings.Set(SETTING_GAMEMODE, HostRequest->ModeNameForAdvertisement, EOnlineComparisonOp::Equals);
	}

	if (!HostRequest->GetMapName().IsEmpty())
	{
		QuickPlaySearch->QuerySettings.Set(SETTING_MAPNAME, HostRequest->GetMapName(), EOnlineComparisonOp::Equals);
	} 
	*/

	// QuickPlaySearch->QuerySettings.Set(SEARCH_DEDICATED_ONLY, true, EOnlineComparisonOp::Equals);
	return QuickPlaySearch;
}

void UEsotericSessionSubsystem::HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage, TWeakObjectPtr<APlayerController> JoiningOrHostingPlayer, TStrongObjectPtr<UEsotericSession_HostSessionRequest> HostRequest)
{
	const int32 ResultCount = SearchSettings->SearchRequest->Results.Num();
	UE_LOG(LogEsotericSession, Log, TEXT("QuickPlay Search Finished %s (Results %d) (Issue: %s)"), bSucceeded ? TEXT("Success") : TEXT("Failed"), ResultCount, *ErrorMessage.ToString());

	//@TODO: We have to check if the error message is empty because some OSS layers report a failure just because there are no sessions.  Please fix with OSS 2.0.
	if (bSucceeded || ErrorMessage.IsEmpty())
	{
		// Join the best search result.
		if (ResultCount > 0)
		{
			//@TODO: We should probably look at ping?  maybe some other factors to find the best.  Idk if they come pre-sorted or not.
			for (UEsotericSession_SearchResult* Result : SearchSettings->SearchRequest->Results)
			{
				JoinSession(JoiningOrHostingPlayer.Get(), Result);
				return;
			}
		}
		else
		{
			HostSession(JoiningOrHostingPlayer.Get(), HostRequest.Get());
		}
	}
	else
	{
		//@TODO: This sucks, need to tell someone.
		NotifySessionInformationUpdated(EEsotericSessionInformationState::OutOfGame);
	}
}

#pragma endregion //QuickPlay

#pragma region Create Session

void UEsotericSessionSubsystem::HostSession(APlayerController* HostingPlayer, UEsotericSession_HostSessionRequest* Request)
{
	if (Request == nullptr)
	{
		SetCreateSessionError(NSLOCTEXT("NetworkErrors", "InvalidRequest", "HostSession passed an invalid request."));
		OnCreateSessionComplete(NAME_None, false);
		return;
	}

	ULocalPlayer* LocalPlayer = (HostingPlayer != nullptr) ? HostingPlayer->GetLocalPlayer() : nullptr;
	if (LocalPlayer == nullptr && !bIsDedicatedServer)
	{
		SetCreateSessionError(NSLOCTEXT("NetworkErrors", "InvalidHostingPlayer", "HostingPlayer is invalid."));
		OnCreateSessionComplete(NAME_None, false);
		return;
	}

	FText OutError;
	if (!Request->ValidateAndLogErrors(OutError))
	{
		SetCreateSessionError(OutError);
		OnCreateSessionComplete(NAME_None, false);
		return;
	}

	if (Request->OnlineMode == EEsotericSessionOnlineMode::Offline)
	{
		if (GetWorld()->GetNetMode() == NM_Client)
		{
			SetCreateSessionError(NSLOCTEXT("NetworkErrors", "CannotHostAsClient", "Cannot host offline game as client."));
			OnCreateSessionComplete(NAME_None, false);
			return;
		}
		else
		{
			// Offline so travel to the specified match URL immediately
			GetWorld()->ServerTravel(Request->ConstructTravelURL());
		}
	}
	else
	{
		CreateOnlineSessionInternal(LocalPlayer, Request);
	}

	NotifySessionInformationUpdated(EEsotericSessionInformationState::InGame, Request->ModeNameForAdvertisement, Request->GetMapName());
}

void UEsotericSessionSubsystem::CreateOnlineSessionInternal(const ULocalPlayer* LocalPlayer, const UEsotericSession_HostSessionRequest* Request)
{
	CreateSessionResult = FOnlineResultInformation();
	PendingTravelURL = Request->ConstructTravelURL();

	const FName SessionName(NAME_GameSession);
	const int32 MaxPlayers = Request->GetMaxPlayers();
	const bool bIsPresence = Request->bUseLobbies; // Using lobbies implies presence

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	check(Sessions);

	FUniqueNetIdPtr UserId;
	if (LocalPlayer)
	{
		UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	}
	else if (bIsDedicatedServer)
	{
		UserId = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(DEDICATED_SERVER_USER_INDEX);
	}

	//@TODO: You can get here on some platforms while trying to do a LAN session, does that require a valid user id?
	if (ensure(UserId.IsValid()))
	{
		HostSettings = MakeShareable(new FEsotericSession_OnlineSessionSettings(Request->OnlineMode == EEsotericSessionOnlineMode::LAN, bIsPresence, MaxPlayers));
		HostSettings->bUseLobbiesIfAvailable = Request->bUseLobbies;
		HostSettings->Set(SETTING_GAMEMODE, Request->ModeNameForAdvertisement, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings->Set(LOBBY_NAME_KEY, Request->LobbyNameForAdvertisement.ToLower(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		HostSettings->Set(SETTING_MAPNAME, Request->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings->Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
		HostSettings->Set(SETTING_SESSION_TEMPLATE_NAME, FString(TEXT("GameSession")), EOnlineDataAdvertisementType::DontAdvertise);

		Sessions->CreateSession(*UserId, SessionName, *HostSettings);
		NotifySessionInformationUpdated(EEsotericSessionInformationState::InGame, Request->ModeNameForAdvertisement, Request->GetMapName());
	}
	else
	{
		OnCreateSessionComplete(SessionName, false);
	}
}

void UEsotericSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogEsotericSession, Log, TEXT("OnCreateSessionComplete(SessionName: %s, bWasSuccessful: %d)"), *SessionName.ToString(), bWasSuccessful);

	FinishSessionCreation(bWasSuccessful);
}

void UEsotericSessionSubsystem::OnRegisterLocalPlayerComplete_CreateSession(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result)
{
	FinishSessionCreation(Result == EOnJoinSessionCompleteResult::Success);
}

void UEsotericSessionSubsystem::FinishSessionCreation(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		//@TODO Synchronize timing of this with join callbacks, modify both places and the comments if plan changes
		CreateSessionResult = FOnlineResultInformation();
		CreateSessionResult.bWasSuccessful = true;

		NotifyCreateSessionComplete(CreateSessionResult);

		// Travel to the specified match URL
		GetWorld()->ServerTravel(PendingTravelURL);
	}
	else
	{
		if (CreateSessionResult.bWasSuccessful || CreateSessionResult.ErrorText.IsEmpty())
		{
			FString ReturnError = TEXT("GenericFailure"); // TODO: No good way to get session error codes
			FText ReturnReason = NSLOCTEXT("NetworkErrors", "CreateSessionFailed", "Failed to create session.");

			CreateSessionResult.bWasSuccessful = false;
			CreateSessionResult.ErrorId = ReturnError;
			CreateSessionResult.ErrorText = ReturnReason;
		}

		UE_LOG(LogEsotericSession, Error, TEXT("FinishSessionCreation(%s): %s"), *CreateSessionResult.ErrorId, *CreateSessionResult.ErrorText.ToString());

		NotifyCreateSessionComplete(CreateSessionResult);
		NotifySessionInformationUpdated(EEsotericSessionInformationState::OutOfGame);
	}
}

void UEsotericSessionSubsystem::NotifyCreateSessionComplete(const FOnlineResultInformation& Result) const
{
	OnCreateSessionCompleteEvent.Broadcast(Result);
	K2_OnCreateSessionCompleteEvent.Broadcast(Result);
}

#pragma endregion //Create Session

#pragma region Join Session

void UEsotericSessionSubsystem::JoinSession(APlayerController* JoiningPlayer, UEsotericSession_SearchResult* Request)
{
	if (Request == nullptr)
	{
		UE_LOG(LogEsotericSession, Error, TEXT("JoinSession passed a null request"));
		return;
	}

	ULocalPlayer* LocalPlayer = (JoiningPlayer != nullptr) ? JoiningPlayer->GetLocalPlayer() : nullptr;
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogEsotericSession, Error, TEXT("JoiningPlayer is invalid"));
		return;
	}

	// Update presence here since we won't have the raw game mode and map name keys after client travel. If joining/travel fails, it is reset to the main menu 
	FString SessionGameMode, SessionMapName;
	bool bEmpty;
	Request->GetStringSetting(SETTING_GAMEMODE, SessionGameMode, bEmpty);
	Request->GetStringSetting(SETTING_MAPNAME, SessionMapName, bEmpty);
	NotifySessionInformationUpdated(EEsotericSessionInformationState::InGame, SessionGameMode, SessionMapName);

	JoinSessionInternal(LocalPlayer, Request);
}

void UEsotericSessionSubsystem::JoinSessionInternal(const ULocalPlayer* LocalPlayer, const UEsotericSession_SearchResult* Request) const
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	check(Sessions);

	Sessions->JoinSession(*LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), NAME_GameSession, Request->Result);
}

void UEsotericSessionSubsystem::InternalTravelToSession(const FName SessionName) const
{
	//@TODO: Ideally we'd use triggering player instead of first (they're all gonna go at once so it probably doesn't matter)
	APlayerController* const PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	if (PlayerController == nullptr)
	{
		FText ReturnReason = NSLOCTEXT("NetworkErrors", "InvalidPlayerController", "Invalid Player Controller");
		UE_LOG(LogEsotericSession, Error, TEXT("InternalTravelToSession(Failed due to %s)"), *ReturnReason.ToString());
		return;
	}

	FString URL;

	// travel to session
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	check(Sessions.IsValid());

	if (!Sessions->GetResolvedConnectString(SessionName, URL))
	{
		FText FailReason = NSLOCTEXT("NetworkErrors", "TravelSessionFailed", "Travel to Session failed.");
		UE_LOG(LogEsotericSession, Error, TEXT("InternalTravelToSession(%s)"), *FailReason.ToString());
		return;
	}

	// Allow modification of the URL prior to travel
	OnPreClientTravelEvent.Broadcast(URL);
		
	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}

void UEsotericSessionSubsystem::NotifyUserRequestedSession(const FPlatformUserId& PlatformUserId, UEsotericSession_SearchResult* RequestedSession, const FOnlineResultInformation& RequestedSessionResult) const
{
	OnUserRequestedSessionEvent.Broadcast(PlatformUserId, RequestedSession, RequestedSessionResult);
	K2_OnUserRequestedSessionEvent.Broadcast(PlatformUserId, RequestedSession, RequestedSessionResult);
}

void UEsotericSessionSubsystem::NotifyJoinSessionComplete(const FOnlineResultInformation& Result) const
{
	OnJoinSessionCompleteEvent.Broadcast(Result);
	K2_OnJoinSessionCompleteEvent.Broadcast(Result);
}

void UEsotericSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) const
{
	FinishJoinSession(Result);
}

void UEsotericSessionSubsystem::OnRegisterJoiningLocalPlayerComplete(const FUniqueNetId& PlayerId, EOnJoinSessionCompleteResult::Type Result) const
{
	FinishJoinSession(Result);
}

void UEsotericSessionSubsystem::FinishJoinSession(EOnJoinSessionCompleteResult::Type Result) const
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		//@TODO Synchronize timing of this with create callbacks, modify both places and the comments if plan changes
		FOnlineResultInformation JoinSessionResult;
		JoinSessionResult.bWasSuccessful = true;
		NotifyJoinSessionComplete(JoinSessionResult);

		InternalTravelToSession(NAME_GameSession);
	}
	else
	{
		FText ReturnReason;
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::SessionIsFull:
			ReturnReason = NSLOCTEXT("NetworkErrors", "SessionIsFull", "Game is full.");
			break;
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			ReturnReason = NSLOCTEXT("NetworkErrors", "SessionDoesNotExist", "Game no longer exists.");
			break;
		default:
			ReturnReason = NSLOCTEXT("NetworkErrors", "JoinSessionFailed", "Join failed.");
			break;
		}

		//@TODO: Error handling
		UE_LOG(LogEsotericSession, Error, TEXT("FinishJoinSession(Failed with Result: %s)"), *ReturnReason.ToString());

		// No FOnlineError to initialize from
		FOnlineResultInformation JoinSessionResult;
		JoinSessionResult.bWasSuccessful = false;
		JoinSessionResult.ErrorId = LexToString(Result); // This is not robust, but there is no extended information available
		JoinSessionResult.ErrorText = ReturnReason;
		NotifyJoinSessionComplete(JoinSessionResult);
		NotifySessionInformationUpdated(EEsotericSessionInformationState::OutOfGame);
	}
}

#pragma endregion //Join Session

#pragma region Find Sessions

void UEsotericSessionSubsystem::FindSessions(APlayerController* SearchingPlayer, UEsotericSession_SearchSessionRequest* Request)
{
	if (Request == nullptr)
	{
		UE_LOG(LogEsotericSession, Error, TEXT("FindSessions passed a null request"));
		return;
	}

	FindSessionsInternal(SearchingPlayer, MakeShared<FEsotericOnlineSearchSettings>(Request));
}

void UEsotericSessionSubsystem::FindSessionsInternal(const APlayerController* SearchingPlayer, const TSharedRef<FEsotericOnlineSearchSettings>& InSearchSettings)
{
	if (SearchSettings.IsValid())
	{
		//@TODO: This is a poor user experience for the API user, we should let the additional search piggyback and
		// just give it the same results as the currently pending one
		// (or enqueue the request and service it when the previous one finishes or fails)
		UE_LOG(LogEsotericSession, Error, TEXT("A previous FindSessions call is still in progress, aborting"));
		SearchSettings->SearchRequest->NotifySearchFinished(false, LOCTEXT("Error_FindSessionAlreadyInProgress", "Session search already in progress"));
	}

	ULocalPlayer* LocalPlayer = (SearchingPlayer != nullptr) ? SearchingPlayer->GetLocalPlayer() : nullptr;
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogEsotericSession, Error, TEXT("SearchingPlayer is invalid"));
		InSearchSettings->SearchRequest->NotifySearchFinished(false, LOCTEXT("Error_FindSessionBadPlayer", "Session search was not provided a local player"));
		return;
	}

	SearchSettings = InSearchSettings;

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	check(Sessions);

	if (!Sessions->FindSessions(*LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), StaticCastSharedRef<FEsotericOnlineSearchSettings>(SearchSettings.ToSharedRef())))
	{
		// Some session search failures will call this delegate inside the function, others will not
		OnFindSessionsComplete(false);
	}
}

void UEsotericSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogEsotericSession, Log, TEXT("OnFindSessionsComplete(bWasSuccessful: %s)"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!SearchSettings.IsValid())
	{
		// This could get called twice for failed session searches, or for a search requested by a different system
		return;
	}

	FEsotericOnlineSearchSettings& SearchSettingsV1 = *StaticCastSharedPtr<FEsotericOnlineSearchSettings>(SearchSettings);
	if (SearchSettingsV1.SearchState == EOnlineAsyncTaskState::InProgress)
	{
		UE_LOG(LogEsotericSession, Error, TEXT("OnFindSessionsComplete called when search is still in progress!"));
		return;
	}

	if (!ensure(SearchSettingsV1.SearchRequest))
	{
		UE_LOG(LogEsotericSession, Error, TEXT("OnFindSessionsComplete called with invalid search request object!"));
		return;
	}

	if (bWasSuccessful)
	{
		SearchSettingsV1.SearchRequest->Results.Reset(SearchSettingsV1.SearchResults.Num());

		for (const FOnlineSessionSearchResult& Result : SearchSettingsV1.SearchResults)
		{
			UEsotericSession_SearchResult* Entry = NewObject<UEsotericSession_SearchResult>(SearchSettingsV1.SearchRequest);
			Entry->Result = Result;
			SearchSettingsV1.SearchRequest->Results.Add(Entry);
			FString OwningUserId = TEXT("Unknown");
			if (Result.Session.OwningUserId.IsValid())
			{
				OwningUserId = Result.Session.OwningUserId->ToString();
			}

			UE_LOG(LogEsotericSession, Log, TEXT("\tFound session (UserId: %s, UserName: %s, NumOpenPrivConns: %d, NumOpenPubConns: %d, Ping: %d ms"),
				*OwningUserId,
				*Result.Session.OwningUserName,
				Result.Session.NumOpenPrivateConnections,
				Result.Session.NumOpenPublicConnections,
				Result.PingInMs
				);
		}
	}
	else
	{
		SearchSettingsV1.SearchRequest->Results.Empty();
	}
	
	SearchSettingsV1.SearchRequest->NotifySearchFinished(bWasSuccessful, bWasSuccessful ? FText() : LOCTEXT("Error_FindSessionFailed", "Find session failed"));
	SearchSettings.Reset();
}

#pragma endregion //Find Sessions


void UEsotericSessionSubsystem::CleanUpSessions()
{
	bWantToDestroyPendingSession = true;
	HostSettings.Reset();
	NotifySessionInformationUpdated(EEsotericSessionInformationState::OutOfGame);

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	check(Sessions);

	EOnlineSessionState::Type SessionState = Sessions->GetSessionState(NAME_GameSession);
	UE_LOG(LogEsotericSession, Log, TEXT("Session state is %s"), EOnlineSessionState::ToString(SessionState));

	if (EOnlineSessionState::InProgress == SessionState)
	{
		UE_LOG(LogEsotericSession, Log, TEXT("Ending session because of return to front end"));
		Sessions->EndSession(NAME_GameSession);
	}
	else if (EOnlineSessionState::Ending == SessionState)
	{
		UE_LOG(LogEsotericSession, Log, TEXT("Waiting for session to end on return to main menu"));
	}
	else if (EOnlineSessionState::Ended == SessionState || EOnlineSessionState::Pending == SessionState)
	{
		UE_LOG(LogEsotericSession, Log, TEXT("Destroying session on return to main menu"));
		Sessions->DestroySession(NAME_GameSession);
	}
	else if (EOnlineSessionState::Starting == SessionState || EOnlineSessionState::Creating == SessionState)
	{
		UE_LOG(LogEsotericSession, Log, TEXT("Waiting for session to start, and then we will end it to return to main menu"));
	}

	bWantToDestroyPendingSession = false;
}


void UEsotericSessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogEsotericSession, Log, TEXT("OnStartSessionComplete(SessionName: %s, bWasSuccessful: %d)"), *SessionName.ToString(), bWasSuccessful);

	if (bWantToDestroyPendingSession)
	{
		CleanUpSessions();
	}
}

void UEsotericSessionSubsystem::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogEsotericSession, Log, TEXT("OnUpdateSessionComplete(SessionName: %s, bWasSuccessful: %s)"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
}

void UEsotericSessionSubsystem::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogEsotericSession, Log, TEXT("OnEndSessionComplete(SessionName: %s, bWasSuccessful: %s)"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
	CleanUpSessions();
}

void UEsotericSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogEsotericSession, Log, TEXT("OnDestroySessionComplete(SessionName: %s, bWasSuccessful: %s)"), *SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));
	bWantToDestroyPendingSession = false;
}

void UEsotericSessionSubsystem::HandleSessionFailure(const FUniqueNetId& NetId, ESessionFailure::Type FailureType)
{
	UE_LOG(LogEsotericSession, Warning, TEXT("UEsotericSessionSubsystem::HandleSessionFailure(NetId: %s, FailureType: %s)"), *NetId.ToDebugString(), LexToString(FailureType));

	//@TODO: Probably need to do a bit more...
}

void UEsotericSessionSubsystem::HandleSessionUserInviteAccepted(const bool bWasSuccessful, const int32 LocalUserIndex, FUniqueNetIdPtr AcceptingUserId, const FOnlineSessionSearchResult& SearchResult)
{
	const FPlatformUserId PlatformUserId = IPlatformInputDeviceMapper::Get().GetPlatformUserForUserIndex(LocalUserIndex);

	UEsotericSession_SearchResult* RequestedSession = nullptr;
	FOnlineResultInformation ResultInfo;
	if (bWasSuccessful)
	{
		RequestedSession = NewObject<UEsotericSession_SearchResult>(this);
		RequestedSession->Result = SearchResult;
		
		JoinSession(UInputDeviceLibrary::GetPlayerControllerFromPlatformUser(PlatformUserId), RequestedSession);
	}
	else
	{
		// No FOnlineError to initialize from
		ResultInfo.bWasSuccessful = false;
		ResultInfo.ErrorId = TEXT("failed"); // This is not robust, but there is no extended information available
		ResultInfo.ErrorText = LOCTEXT("Error_SessionUserInviteAcceptedFailed", "Failed to handle the join request");
	}
	NotifyUserRequestedSession(PlatformUserId, RequestedSession, ResultInfo);
}

void UEsotericSessionSubsystem::SetCreateSessionError(const FText& ErrorText)
{
	CreateSessionResult.bWasSuccessful = false;
	CreateSessionResult.ErrorId = TEXT("InternalFailure");

	// TODO May want to replace with a generic error text in shipping builds depending on how much data you want to give users
	CreateSessionResult.ErrorText = ErrorText;
}


void UEsotericSessionSubsystem::TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString)
{
	// The delegate for this is global, but PIE can have more than one game instance, so make
	// sure it's being raised for the same world this game instance subsystem is associated with
	if (World != GetWorld())
	{
		return;
	}

	UE_LOG(LogEsotericSession,
		Warning,
		TEXT("TravelLocalSessionFailure(World: %s, FailureType: %s, ReasonString: %s)"),
		*GetPathNameSafe(World),
		ETravelFailure::ToString(FailureType),
		*ReasonString);
}

void UEsotericSessionSubsystem::HandlePostLoadMap(UWorld* World)
{
	// Ignore null worlds.
	if (!World)
	{
		return;
	}

	// Ignore any world that isn't part of this game instance, which can be the case in the editor.
	if (World->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// We don't care about updating the session unless the world type is game/pie.
	if (!(World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
	{
		return;
	}
	
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	// If we're hosting a session, update the advertised map name.
	if (HostSettings.IsValid())
	{
		// This needs to be the full package path to match the host GetMapName function, World→GetMapName is currently the short name
		HostSettings->Set(SETTING_MAPNAME, UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()), EOnlineDataAdvertisementType::ViaOnlineService);

		const FName SessionName(NAME_GameSession);
		SessionInterface->UpdateSession(SessionName, *HostSettings, true);
	}
}

#undef LOCTEXT_NAMESPACE
