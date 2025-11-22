// Fill out your copyright notice in the Description page of Project Settings.


#include "EsotericUserSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EsotericUserSubsystem)

DECLARE_LOG_CATEGORY_EXTERN(LogEsotericUser, Log, All);
DEFINE_LOG_CATEGORY(LogEsotericUser);

UE_DEFINE_GAMEPLAY_TAG(FEsotericUserTags::SystemMessage_Error, "SystemMessage.Error");
UE_DEFINE_GAMEPLAY_TAG(FEsotericUserTags::SystemMessage_Warning, "SystemMessage.Warning");
UE_DEFINE_GAMEPLAY_TAG(FEsotericUserTags::SystemMessage_Display, "SystemMessage.Display");
UE_DEFINE_GAMEPLAY_TAG(FEsotericUserTags::SystemMessage_Error_InitializeLocalPlayerFailed, "SystemMessage.Error.InitializeLocalPlayerFailed");

UE_DEFINE_GAMEPLAY_TAG(FEsotericUserTags::Platform_Trait_RequiresStrictControllerMapping, "Platform.Trait.RequiresStrictControllerMapping");
UE_DEFINE_GAMEPLAY_TAG(FEsotericUserTags::Platform_Trait_SingleOnlineUser, "Platform.Trait.SingleOnlineUser");

#pragma region Esoteric User Info

UEsotericUserInfo::FCachedData* UEsotericUserInfo::GetCachedData(EEsotericUserOnlineContext Context)
{
	// Look up directly, game has a separate cache than default
	if (FCachedData* FoundData = CachedDataMap.Find(Context))
	{
		return FoundData;
	}

	// Now try system resolution
	const UEsotericUserSubsystem* Subsystem = GetSubsystem();

	EEsotericUserOnlineContext ResolvedContext = Subsystem->ResolveOnlineContext(Context);
	return CachedDataMap.Find(ResolvedContext);
}

const UEsotericUserInfo::FCachedData* UEsotericUserInfo::GetCachedData(const EEsotericUserOnlineContext Context) const
{
	return const_cast<UEsotericUserInfo*>(this)->GetCachedData(Context);
}

void UEsotericUserInfo::UpdateCachedPrivilegeResult(const EEsotericUserPrivilege Privilege, const EEsotericUserPrivilegeResult Result, const EEsotericUserOnlineContext Context)
{
	// This should only be called with a resolved and valid type
	FCachedData* GameCache = GetCachedData(EEsotericUserOnlineContext::Game);
	FCachedData* ContextCache = GetCachedData(Context);

	if (!ensure(GameCache && ContextCache))
	{
		// Should always be valid
		return;
	}

	// Update direct cache first
	ContextCache->CachedPrivileges.Add(Privilege, Result);

	if (GameCache != ContextCache)
	{
		// Look for another context to merge into game
		EEsotericUserPrivilegeResult GameContextResult = Result;
		EEsotericUserPrivilegeResult OtherContextResult = EEsotericUserPrivilegeResult::Available;
		for (TPair<EEsotericUserOnlineContext, FCachedData>& Pair : CachedDataMap)
		{
			if (&Pair.Value != ContextCache && &Pair.Value != GameCache)
			{
				if (const EEsotericUserPrivilegeResult* FoundResult = Pair.Value.CachedPrivileges.Find(Privilege))
				{
					OtherContextResult = *FoundResult;
				}
				else
				{
					OtherContextResult = EEsotericUserPrivilegeResult::Unknown;
				}
				break;
			}
		}

		if (GameContextResult == EEsotericUserPrivilegeResult::Available && OtherContextResult != EEsotericUserPrivilegeResult::Available)
		{
			// Other context is worse, use that
			GameContextResult = OtherContextResult;
		}

		GameCache->CachedPrivileges.Add(Privilege, GameContextResult);
	}
}

void UEsotericUserInfo::UpdateCachedNetId(const FUniqueNetIdRepl& NewId, const EEsotericUserOnlineContext Context)
{
	if (FCachedData* ContextCache = GetCachedData(Context); ensure(ContextCache))
	{
		ContextCache->CachedNetId = NewId;
	}
	// We don't merge the ids because of how guests work
}

UEsotericUserSubsystem* UEsotericUserInfo::GetSubsystem() const
{
	return Cast<UEsotericUserSubsystem>(GetOuter());
}

bool UEsotericUserInfo::IsLoggedIn() const
{
	return (InitializationState == EEsotericUserInitializationState::LoggedInLocalOnly || InitializationState == EEsotericUserInitializationState::LoggedInOnline);
}

bool UEsotericUserInfo::IsDoingLogin() const
{
	return InitializationState == EEsotericUserInitializationState::DoingInitialLogin || InitializationState == EEsotericUserInitializationState::DoingNetworkLogin;
}

EEsotericUserPrivilegeResult UEsotericUserInfo::GetCachedPrivilegeResult(EEsotericUserPrivilege Privilege, EEsotericUserOnlineContext Context) const
{
	if (const FCachedData* FoundCached = GetCachedData(Context))
	{
		if (const EEsotericUserPrivilegeResult* FoundResult = FoundCached->CachedPrivileges.Find(Privilege))
		{
			return *FoundResult;
		}
	}
	return EEsotericUserPrivilegeResult::Unknown;
}

EEsotericUserAvailability UEsotericUserInfo::GetPrivilegeAvailability(EEsotericUserPrivilege Privilege) const
{
	// Bad feature or user
	if ((int32)Privilege < 0 || (int32)Privilege >= (int32)EEsotericUserPrivilege::Invalid_Count || InitializationState == EEsotericUserInitializationState::Invalid)
	{
		return EEsotericUserAvailability::Invalid;
	}

	const EEsotericUserPrivilegeResult CachedResult = GetCachedPrivilegeResult(Privilege, EEsotericUserOnlineContext::Game);

	// First handle explicit failures
	switch (CachedResult)
	{
	case EEsotericUserPrivilegeResult::LicenseInvalid:
	case EEsotericUserPrivilegeResult::VersionOutdated:
	case EEsotericUserPrivilegeResult::AgeRestricted:
		return EEsotericUserAvailability::AlwaysUnavailable;

	case EEsotericUserPrivilegeResult::NetworkConnectionUnavailable:
	case EEsotericUserPrivilegeResult::AccountTypeRestricted:
	case EEsotericUserPrivilegeResult::AccountUseRestricted:
	case EEsotericUserPrivilegeResult::PlatformFailure:
		return EEsotericUserAvailability::CurrentlyUnavailable;

	default:
		break;
	}

	if (bIsGuest)
	{
		// Guests can only play, cannot use online features
		if (Privilege == EEsotericUserPrivilege::CanPlay)
		{
			return EEsotericUserAvailability::NowAvailable;
		}
		else
		{
			return EEsotericUserAvailability::AlwaysUnavailable;
		}
	}

	// Check network status
	if (Privilege == EEsotericUserPrivilege::CanPlayOnline ||
		Privilege == EEsotericUserPrivilege::CanUseCrossPlay ||
		Privilege == EEsotericUserPrivilege::CanCommunicateViaTextOnline ||
		Privilege == EEsotericUserPrivilege::CanCommunicateViaVoiceOnline)
	{
		if (const UEsotericUserSubsystem* Subsystem = GetSubsystem(); ensure(Subsystem) && !Subsystem->HasOnlineConnection(EEsotericUserOnlineContext::Game))
		{
			return EEsotericUserAvailability::CurrentlyUnavailable;
		}
	}

	if (InitializationState == EEsotericUserInitializationState::FailedtoLogin)
	{
		// Failed a prior login attempt
		return EEsotericUserAvailability::CurrentlyUnavailable;
	}
	else if (InitializationState == EEsotericUserInitializationState::Unknown || InitializationState == EEsotericUserInitializationState::DoingInitialLogin)
	{
		// Haven't logged in yet
		return EEsotericUserAvailability::PossiblyAvailable;
	}
	else if (InitializationState == EEsotericUserInitializationState::LoggedInLocalOnly || InitializationState == EEsotericUserInitializationState::DoingNetworkLogin)
	{
		// Local login succeeded so play checks are valid
		if (Privilege == EEsotericUserPrivilege::CanPlay && CachedResult == EEsotericUserPrivilegeResult::Available)
		{
			return EEsotericUserAvailability::NowAvailable;
		}

		// Haven't logged in online yet
		return EEsotericUserAvailability::PossiblyAvailable;
	}
	else if (InitializationState == EEsotericUserInitializationState::LoggedInOnline)
	{
		// Fully logged in
		if (CachedResult == EEsotericUserPrivilegeResult::Available)
		{
			return EEsotericUserAvailability::NowAvailable;
		}

		// Failed for other reason
		return EEsotericUserAvailability::CurrentlyUnavailable;
	}

	return EEsotericUserAvailability::Unknown;
}

FUniqueNetIdRepl UEsotericUserInfo::GetNetId(const EEsotericUserOnlineContext Context) const
{
	if (const FCachedData* FoundCached = GetCachedData(Context))
	{
		return FoundCached->CachedNetId;
	}

	return FUniqueNetIdRepl();
}

FString UEsotericUserInfo::GetNickname() const
{
	if (bIsGuest)
	{
		return NSLOCTEXT("EsotericUser", "GuestNickname", "Guest").ToString();
	}

	if (const UEsotericUserSubsystem* Subsystem = GetSubsystem(); ensure(Subsystem))
	{
		if (const IOnlineIdentity* Identity = Subsystem->GetOnlineIdentity(EEsotericUserOnlineContext::Game); ensure(Identity))
		{
			return Identity->GetPlayerNickname(GetPlatformUserIndex());
		}
	}
	return FString();
}

FString UEsotericUserInfo::GetDebugString() const
{
	const FUniqueNetIdRepl NetId = GetNetId();
	return NetId.ToDebugString();
}

FPlatformUserId UEsotericUserInfo::GetPlatformUserId() const
{
	return PlatformUser;
}

int32 UEsotericUserInfo::GetPlatformUserIndex() const
{
	// Convert our platform id to index

	if (const UEsotericUserSubsystem* Subsystem = GetSubsystem(); ensure(Subsystem))
	{
		return Subsystem->GetPlatformUserIndexForId(PlatformUser);
	}

	return INDEX_NONE;
}

#pragma endregion //Esoteric User Info



//////////////////////////////////////
///EsotericUserSubsystem

void UEsotericUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create our OSS wrappers
	CreateOnlineContexts();

	BindOnlineDelegates();

	const IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	DeviceMapper.GetOnInputDeviceConnectionChange().AddUObject(this, &ThisClass::HandleInputDeviceConnectionChanged);

	// Matches the engine default
	SetMaxLocalPlayers(4);

	ResetUserState();

	const UGameInstance* GameInstance = GetGameInstance();
	bIsDedicatedServer = GameInstance->IsDedicatedServerInstance();
}

void UEsotericUserSubsystem::CreateOnlineContexts()
{
	// First initialize default
	DefaultContextInternal = new FOnlineContextCache();

	DefaultContextInternal->OnlineSubsystem = Online::GetSubsystem(GetWorld());
	check(DefaultContextInternal->OnlineSubsystem);
	DefaultContextInternal->IdentityInterface = DefaultContextInternal->OnlineSubsystem->GetIdentityInterface();
	check(DefaultContextInternal->IdentityInterface.IsValid());

	if (IOnlineSubsystem* PlatformSub = IOnlineSubsystem::GetByPlatform(); PlatformSub && DefaultContextInternal->OnlineSubsystem != PlatformSub)
	{
		// Set up the optional platform service if it exists
		PlatformContextInternal = new FOnlineContextCache();
		PlatformContextInternal->OnlineSubsystem = PlatformSub;
		PlatformContextInternal->IdentityInterface = PlatformSub->GetIdentityInterface();
		check(PlatformContextInternal->IdentityInterface.IsValid());
	}

	// Explicit external services can be set up after if needed
}

void UEsotericUserSubsystem::Deinitialize()
{
	DestroyOnlineContexts();

	const IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	DeviceMapper.GetOnInputDeviceConnectionChange().RemoveAll(this);

	LocalUserInfos.Reset();
	ActiveLoginRequests.Reset();

	Super::Deinitialize();
}

void UEsotericUserSubsystem::DestroyOnlineContexts()
{
	// All cached shared ptrs must be cleared here
	if (ServiceContextInternal && ServiceContextInternal != DefaultContextInternal)
	{
		delete ServiceContextInternal;
	}
	if (PlatformContextInternal && PlatformContextInternal != DefaultContextInternal)
	{
		delete PlatformContextInternal;
	}
	if (DefaultContextInternal)
	{
		delete DefaultContextInternal;
	}

	ServiceContextInternal = PlatformContextInternal = DefaultContextInternal = nullptr;
}

UEsotericUserInfo* UEsotericUserSubsystem::CreateLocalUserInfo(const int32 LocalPlayerIndex)
{
	UEsotericUserInfo* NewUser = nullptr;
	if (ensure(!LocalUserInfos.Contains(LocalPlayerIndex)))
	{
		NewUser = NewObject<UEsotericUserInfo>(this);
		NewUser->LocalPlayerIndex = LocalPlayerIndex;
		NewUser->InitializationState = EEsotericUserInitializationState::Unknown;

		// Always create game and default cache
		NewUser->CachedDataMap.Add(EEsotericUserOnlineContext::Game, UEsotericUserInfo::FCachedData());
		NewUser->CachedDataMap.Add(EEsotericUserOnlineContext::Default, UEsotericUserInfo::FCachedData());

		// Add platform if needed
		if (HasSeparatePlatformContext())
		{
			NewUser->CachedDataMap.Add(EEsotericUserOnlineContext::Platform, UEsotericUserInfo::FCachedData());
		}

		LocalUserInfos.Add(LocalPlayerIndex, NewUser);
	}
	return NewUser;
}

bool UEsotericUserSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass
	return ChildClasses.Num() == 0;
}

void UEsotericUserSubsystem::BindOnlineDelegates()
{
	EEsotericUserOnlineContext ServiceType = ResolveOnlineContext(EEsotericUserOnlineContext::ServiceOrDefault);
	EEsotericUserOnlineContext PlatformType = ResolveOnlineContext(EEsotericUserOnlineContext::PlatformOrDefault);
	FOnlineContextCache* ServiceContext = GetContextCache(ServiceType);
	FOnlineContextCache* PlatformContext = GetContextCache(PlatformType);
	check(ServiceContext && ServiceContext->OnlineSubsystem && PlatformContext && PlatformContext->OnlineSubsystem);
	// Connection delegates need to listen for both systems

	ServiceContext->OnlineSubsystem->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleNetworkConnectionStatusChanged, ServiceType));
	ServiceContext->CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;

	for (int32 PlayerIdx = 0; PlayerIdx < MAX_LOCAL_PLAYERS; PlayerIdx++)
	{
		ServiceContext->IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(PlayerIdx, FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleIdentityLoginStatusChanged, ServiceType));
		ServiceContext->IdentityInterface->AddOnLoginCompleteDelegate_Handle(PlayerIdx, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleUserLoginCompleted, ServiceType));
	}

	if (ServiceType != PlatformType)
	{
		PlatformContext->OnlineSubsystem->AddOnConnectionStatusChangedDelegate_Handle(FOnConnectionStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleNetworkConnectionStatusChanged, PlatformType));
		PlatformContext->CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;

		for (int32 PlayerIdx = 0; PlayerIdx < MAX_LOCAL_PLAYERS; PlayerIdx++)
		{
			PlatformContext->IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(PlayerIdx, FOnLoginStatusChangedDelegate::CreateUObject(this, &ThisClass::HandleIdentityLoginStatusChanged, PlatformType));
			PlatformContext->IdentityInterface->AddOnLoginCompleteDelegate_Handle(PlayerIdx, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleUserLoginCompleted, PlatformType));
		}
	}

	// Hardware change delegates only listen to platform
	PlatformContext->IdentityInterface->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &ThisClass::HandleControllerPairingChanged));
}

void UEsotericUserSubsystem::LogOutLocalUser(const FPlatformUserId PlatformUser)
{
	// Don't need to do anything if the user has never logged in fully or is in the process of logging in
	if (UEsotericUserInfo* UserInfo = ModifyInfo(GetUserInfoForPlatformUser(PlatformUser)); UserInfo && (UserInfo->InitializationState == EEsotericUserInitializationState::LoggedInLocalOnly || UserInfo->InitializationState == EEsotericUserInitializationState::LoggedInOnline))
	{
		const EEsotericUserAvailability OldAvailablity = UserInfo->GetPrivilegeAvailability(EEsotericUserPrivilege::CanPlay);

		UserInfo->InitializationState = EEsotericUserInitializationState::FailedtoLogin;

		// This will broadcast the game delegate
		HandleChangedAvailability(UserInfo, EEsotericUserPrivilege::CanPlay, OldAvailablity);
	}
}

bool UEsotericUserSubsystem::AutoLogin(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	UE_LOG(LogEsotericUser, Log, TEXT("Player AutoLogin requested - UserIdx:%d, Privilege:%d, Context:%d"),
	PlatformUser.GetInternalId(),
	(int32)Request->DesiredPrivilege,
	(int32)Request->DesiredContext);

	return System->IdentityInterface->AutoLogin(GetPlatformUserIndexForId(PlatformUser));
}

bool UEsotericUserSubsystem::ShowLoginUI(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	UE_LOG(LogEsotericUser, Log, TEXT("Player LoginUI requested - UserIdx:%d, Privilege:%d, Context:%d"),
	PlatformUser.GetInternalId(),
	(int32)Request->DesiredPrivilege,
	(int32)Request->DesiredContext);

	if (const IOnlineExternalUIPtr ExternalUI = System->OnlineSubsystem->GetExternalUIInterface(); ExternalUI.IsValid())
	{
		// TODO Unclear which flags should be set
		return ExternalUI->ShowLoginUI(GetPlatformUserIndexForId(PlatformUser), false, false, FOnLoginUIClosedDelegate::CreateUObject(this, &ThisClass::HandleOnLoginUIClosed, Request->CurrentContext));
	}
	return false;
}

bool UEsotericUserSubsystem::QueryUserPrivilege(FOnlineContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	// Start query on unknown or failure
	const EUserPrivileges::Type OSSPrivilege = ConvertOSSPrivilege(Request->DesiredPrivilege);

	const FUniqueNetIdRepl CurrentId = GetLocalUserNetId(PlatformUser, Request->CurrentContext);
	check(CurrentId.IsValid());
	const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate Delegate = IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateUObject(this, &ThisClass::HandleCheckPrivilegesComplete, Request->DesiredPrivilege, Request->UserInfo, Request->CurrentContext);
	System->IdentityInterface->GetUserPrivilege(*CurrentId, OSSPrivilege, Delegate);

	// This may immediately succeed and reenter this function, so we have to return
	return true;
}

IOnlineSubsystem* UEsotericUserSubsystem::GetOnlineSubsystem(const EEsotericUserOnlineContext Context) const
{
	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->OnlineSubsystem;
	}

	return nullptr;
}

IOnlineIdentity* UEsotericUserSubsystem::GetOnlineIdentity(EEsotericUserOnlineContext Context) const
{
	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->IdentityInterface.Get();
	}

	return nullptr;
}

FName UEsotericUserSubsystem::GetOnlineSubsystemName(EEsotericUserOnlineContext Context) const
{
	if (const IOnlineSubsystem* SubSystem = GetOnlineSubsystem(Context))
	{
		return SubSystem->GetSubsystemName();
	}

	return NAME_None;
}

EOnlineServerConnectionStatus::Type UEsotericUserSubsystem::GetConnectionStatus(const EEsotericUserOnlineContext Context) const
{
	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->CurrentConnectionStatus;
	}

	return EOnlineServerConnectionStatus::ServiceUnavailable;
}

bool UEsotericUserSubsystem::HasOnlineConnection(const EEsotericUserOnlineContext Context) const
{
	EOnlineServerConnectionStatus::Type ConnectionType = GetConnectionStatus(Context);

	if (ConnectionType == EOnlineServerConnectionStatus::Normal || ConnectionType == EOnlineServerConnectionStatus::Connected)
	{
		return true;
	}

	return false;
}

ELoginStatusType UEsotericUserSubsystem::GetLocalUserLoginStatus(const FPlatformUserId PlatformUser, const EEsotericUserOnlineContext Context) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return ELoginStatusType::NotLoggedIn;
	}

	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return System->IdentityInterface->GetLoginStatus(GetPlatformUserIndexForId(PlatformUser));
	}

	return ELoginStatusType::NotLoggedIn;
}

FUniqueNetIdRepl UEsotericUserSubsystem::GetLocalUserNetId(const FPlatformUserId PlatformUser, const EEsotericUserOnlineContext Context) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return FUniqueNetIdRepl();
	}

	if (const FOnlineContextCache* System = GetContextCache(Context))
	{
		return FUniqueNetIdRepl(System->IdentityInterface->GetUniquePlayerId(GetPlatformUserIndexForId(PlatformUser)));
	}

	return FUniqueNetIdRepl();
}


void UEsotericUserSubsystem::SendSystemMessage(const FGameplayTag MessageType, const FText TitleText, const FText BodyText)
{
	OnHandleSystemMessage.Broadcast(MessageType, TitleText, BodyText);
}

void UEsotericUserSubsystem::SetMaxLocalPlayers(const int32 InMaxLocalPLayers)
{
	if (ensure(InMaxLocalPLayers >= 1))
	{
		// We can have more local players than MAX_LOCAL_PLAYERS, the rest are treated as guests
		MaxNumberOfLocalPlayers = InMaxLocalPLayers;

		const UGameInstance* GameInstance = GetGameInstance();

		if (UGameViewportClient* ViewportClient = GameInstance ? GameInstance->GetGameViewportClient() : nullptr)
		{
			ViewportClient->MaxSplitscreenPlayers = MaxNumberOfLocalPlayers;
		}
	}
}

int32 UEsotericUserSubsystem::GetMaxLocalPlayers() const
{
	return MaxNumberOfLocalPlayers;
}

int32 UEsotericUserSubsystem::GetNumLocalPlayers() const
{
	if (const UGameInstance* GameInstance = GetGameInstance(); ensure(GameInstance))
	{
		return GameInstance->GetNumLocalPlayers();
	}
	return 1;
}

EEsotericUserInitializationState UEsotericUserSubsystem::GetLocalPlayerInitializationState(const int32 LocalPlayerIndex) const
{
	if (const UEsotericUserInfo* UserInfo = GetUserInfoForLocalPlayerIndex(LocalPlayerIndex))
	{
		return UserInfo->InitializationState;
	}

	if (LocalPlayerIndex < 0 || LocalPlayerIndex >= GetMaxLocalPlayers())
	{
		return EEsotericUserInitializationState::Invalid;
	}

	return EEsotericUserInitializationState::Unknown;
}

bool UEsotericUserSubsystem::TryToInitializeForLocalPlay(int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
	if (!PrimaryInputDevice.IsValid())
	{
		// Set to default device
		PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	}

	FEsotericUserInitializeParams Params;
	Params.LocalPlayerIndex = LocalPlayerIndex;
	Params.PrimaryInputDevice = PrimaryInputDevice;
	Params.bCanUseGuestLogin = bCanUseGuestLogin;
	Params.bCanCreateNewLocalPlayer = true;
	Params.RequestedPrivilege = EEsotericUserPrivilege::CanPlay;

	return TryToInitializeUser(Params);
}

bool UEsotericUserSubsystem::TryToLoginForOnlinePlay(int32 LocalPlayerIndex)
{
	FEsotericUserInitializeParams Params;
	Params.LocalPlayerIndex = LocalPlayerIndex;
	Params.bCanCreateNewLocalPlayer = false;
	Params.RequestedPrivilege = EEsotericUserPrivilege::CanPlayOnline;

	return TryToInitializeUser(Params);
}

bool UEsotericUserSubsystem::TryToInitializeUser(FEsotericUserInitializeParams Params)
{
	if (Params.LocalPlayerIndex < 0 || (!Params.bCanCreateNewLocalPlayer && Params.LocalPlayerIndex >= GetNumLocalPlayers()))
	{
		if (!bIsDedicatedServer)
		{
			UE_LOG(LogEsotericUser, Error, TEXT("TryToInitializeUser %d failed with current %d and max %d, invalid index"), 
				Params.LocalPlayerIndex, GetNumLocalPlayers(), GetMaxLocalPlayers());
			return false;
		}
	}

	if (Params.LocalPlayerIndex > GetNumLocalPlayers() || Params.LocalPlayerIndex >= GetMaxLocalPlayers())
	{
		UE_LOG(LogEsotericUser, Error, TEXT("TryToInitializeUser %d failed with current %d and max %d, can only create in order up to max players"), 
			Params.LocalPlayerIndex, GetNumLocalPlayers(), GetMaxLocalPlayers());
		return false;
	}

	// Fill in platform user and input device if needed
	if (Params.ControllerId != INDEX_NONE && (!Params.PrimaryInputDevice.IsValid() || !Params.PlatformUser.IsValid()))
	{
		IPlatformInputDeviceMapper::Get().RemapControllerIdToPlatformUserAndDevice(Params.ControllerId, Params.PlatformUser, Params.PrimaryInputDevice);
	}

	if (Params.PrimaryInputDevice.IsValid() && !Params.PlatformUser.IsValid())
	{
		Params.PlatformUser = GetPlatformUserIdForInputDevice(Params.PrimaryInputDevice);
	}
	else if (Params.PlatformUser.IsValid() && !Params.PrimaryInputDevice.IsValid())
	{
		Params.PrimaryInputDevice = GetPrimaryInputDeviceForPlatformUser(Params.PlatformUser);
	}

	UEsotericUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex));
	UEsotericUserInfo* LocalUserInfoForController = ModifyInfo(GetUserInfoForInputDevice(Params.PrimaryInputDevice));

	if (LocalUserInfoForController && LocalUserInfo && LocalUserInfoForController != LocalUserInfo)
	{
		UE_LOG(LogEsotericUser, Error, TEXT("TryToInitializeUser %d failed because controller %d is already assigned to player %d"),
			Params.LocalPlayerIndex, Params.PrimaryInputDevice.GetId(), LocalUserInfoForController->LocalPlayerIndex);
		return false;
	}

	if (Params.LocalPlayerIndex == 0 && Params.bCanUseGuestLogin)
	{
		UE_LOG(LogEsotericUser, Error, TEXT("TryToInitializeUser failed because player 0 cannot be a guest"));
		return false;
	}

	if (!LocalUserInfo)
	{
		LocalUserInfo = CreateLocalUserInfo(Params.LocalPlayerIndex);
	}
	else
	{
		// Copy from existing user info
		if (!Params.PrimaryInputDevice.IsValid())
		{
			Params.PrimaryInputDevice = LocalUserInfo->PrimaryInputDevice;
		}

		if (!Params.PlatformUser.IsValid())
		{
			Params.PlatformUser = LocalUserInfo->PlatformUser;
		}
	}
	
	if (LocalUserInfo->InitializationState != EEsotericUserInitializationState::Unknown && LocalUserInfo->InitializationState != EEsotericUserInitializationState::FailedtoLogin)
	{
		// Not allowed to change parameters during login
		if (LocalUserInfo->PrimaryInputDevice != Params.PrimaryInputDevice || LocalUserInfo->PlatformUser != Params.PlatformUser || LocalUserInfo->bCanBeGuest != Params.bCanUseGuestLogin)
		{
			UE_LOG(LogEsotericUser, Error, TEXT("TryToInitializeUser failed because player %d has already started the login process with diffrent settings!"), Params.LocalPlayerIndex);
			return false;
		}
	}

	// Set desired index now so if it creates a player it knows what controller to use
	LocalUserInfo->PrimaryInputDevice = Params.PrimaryInputDevice;
	LocalUserInfo->PlatformUser = Params.PlatformUser;
	LocalUserInfo->bCanBeGuest = Params.bCanUseGuestLogin;
	RefreshLocalUserInfo(LocalUserInfo);

	// Either doing an initial or network login
	if (LocalUserInfo->GetPrivilegeAvailability(EEsotericUserPrivilege::CanPlay) == EEsotericUserAvailability::NowAvailable && Params.RequestedPrivilege == EEsotericUserPrivilege::CanPlayOnline)
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::DoingNetworkLogin;
	}
	else
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::DoingInitialLogin;
	}

	LoginLocalUser(LocalUserInfo, Params.RequestedPrivilege, Params.OnlineContext, FOnLocalUserLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginForUserInitialize, Params));

	return true;
}

void UEsotericUserSubsystem::ListenForLoginKeyInput(TArray<FKey> AnyUserKeys, TArray<FKey> NewUserKeys, FEsotericUserInitializeParams Params)
{
	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();
	if (ensure(ViewportClient))
	{
		const bool bIsMapped = LoginKeysForAnyUser.Num() > 0 || LoginKeysForNewUser.Num() > 0;
		const bool bShouldBeMapped = AnyUserKeys.Num() > 0 || NewUserKeys.Num() > 0;

		if (bIsMapped && !bShouldBeMapped)
		{
			// Set it back to wrapped handler
			ViewportClient->OnOverrideInputKey() = WrappedInputKeyHandler;
			WrappedInputKeyHandler.Unbind();
		}
		else if (!bIsMapped && bShouldBeMapped)
		{
			// Set up a wrapped handler
			WrappedInputKeyHandler = ViewportClient->OnOverrideInputKey();
			ViewportClient->OnOverrideInputKey().BindUObject(this, &UEsotericUserSubsystem::OverrideInputKeyForLogin);
		}

		LoginKeysForAnyUser = AnyUserKeys;
		LoginKeysForNewUser = NewUserKeys;

		if (bShouldBeMapped)
		{
			ParamsForLoginKey = Params;
		}
		else
		{
			ParamsForLoginKey = FEsotericUserInitializeParams();
		}
	}
}

bool UEsotericUserSubsystem::CancelUserInitialization(int32 LocalPlayerIndex)
{
	UEsotericUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(LocalPlayerIndex));
	if (!LocalUserInfo)
	{
		return false;
	}

	if (!LocalUserInfo->IsDoingLogin())
	{
		return false;
	}

	// Remove from login queue
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		if (Request->UserInfo.IsValid() && Request->UserInfo->LocalPlayerIndex == LocalPlayerIndex)
		{
			ActiveLoginRequests.Remove(Request);
		}
	}

	// Set state with best guess
	if (LocalUserInfo->InitializationState == EEsotericUserInitializationState::DoingNetworkLogin)
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::LoggedInLocalOnly;
	}
	else
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::FailedtoLogin;
	}

	return true;
}

bool UEsotericUserSubsystem::TryToLogOutUser(int32 LocalPlayerIndex, bool bDestroyPlayer)
{
	UGameInstance* GameInstance = GetGameInstance();
	
	if (!ensure(GameInstance))
	{
		return false;
	}

	if (LocalPlayerIndex == 0 && bDestroyPlayer)
	{
		UE_LOG(LogEsotericUser, Error, TEXT("TryToLogOutUser cannot destroy player 0"));
		return false;
	}

	CancelUserInitialization(LocalPlayerIndex);
	
	UEsotericUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(LocalPlayerIndex));
	if (!LocalUserInfo)
	{
		UE_LOG(LogEsotericUser, Warning, TEXT("TryToLogOutUser failed to log out user %i because they are not logged in"), LocalPlayerIndex);
		return false;
	}

	FPlatformUserId UserId = LocalUserInfo->PlatformUser;
	if (IsRealPlatformUser(UserId))
	{
		// Currently this does not do platform logout in case they want to log back in immediately after
		UE_LOG(LogEsotericUser, Log, TEXT("TryToLogOutUser succeeded for real platform user %d"), UserId.GetInternalId());

		LogOutLocalUser(UserId);
	}
	else if (ensure(LocalUserInfo->bIsGuest))
	{
		// For guest users just delete it
		UE_LOG(LogEsotericUser, Log, TEXT("TryToLogOutUser succeeded for guest player index %d"), LocalPlayerIndex);

		LocalUserInfos.Remove(LocalPlayerIndex);
	}

	if (bDestroyPlayer)
	{
		ULocalPlayer* ExistingPlayer = GameInstance->FindLocalPlayerFromPlatformUserId(UserId);

		if (ExistingPlayer)
		{
			GameInstance->RemoveLocalPlayer(ExistingPlayer);
		}
	}

	return true;
}

void UEsotericUserSubsystem::ResetUserState()
{
	// Manually purge existing info objects
	for (TPair<int32, UEsotericUserInfo*> Pair : LocalUserInfos)
	{
		if (Pair.Value)
		{
			Pair.Value->MarkAsGarbage();
		}
	}

	LocalUserInfos.Reset();

	// Cancel in-progress logins
	ActiveLoginRequests.Reset();

	// Create player info for id 0
	UEsotericUserInfo* FirstUser = CreateLocalUserInfo(0);

	FirstUser->PlatformUser = IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser();
	FirstUser->PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(FirstUser->PlatformUser);

	// TODO: Schedule a refresh of player 0 for next frame?
	RefreshLocalUserInfo(FirstUser);
}

bool UEsotericUserSubsystem::OverrideInputKeyForLogin(FInputKeyEventArgs& EventArgs)
{
	int32 NextLocalPlayerIndex = INDEX_NONE;

	const UEsotericUserInfo* MappedUser = GetUserInfoForInputDevice(EventArgs.InputDevice);
	if (EventArgs.Event == IE_Pressed)
	{
		if (MappedUser == nullptr || !MappedUser->IsLoggedIn())
		{
			if (MappedUser)
			{
				NextLocalPlayerIndex = MappedUser->LocalPlayerIndex;
			}
			else
			{
				// Find next player
				for (int32 i = 0; i < MaxNumberOfLocalPlayers; i++)
				{
					if (GetLocalPlayerInitializationState(i) == EEsotericUserInitializationState::Unknown)
					{
						NextLocalPlayerIndex = i;
						break;
					}
				}
			}

			if (NextLocalPlayerIndex != INDEX_NONE)
			{
				if (LoginKeysForAnyUser.Contains(EventArgs.Key))
				{
					// If we're in the middle of logging in just return true to ignore platform-specific input
					if (MappedUser && MappedUser->IsDoingLogin())
					{
						return true;
					}

					// Press start screen
					FEsotericUserInitializeParams NewParams = ParamsForLoginKey;
					NewParams.LocalPlayerIndex = NextLocalPlayerIndex;
					NewParams.PrimaryInputDevice = EventArgs.InputDevice;

					return TryToInitializeUser(NewParams);
				}

				// See if this controller id is mapped
				MappedUser = GetUserInfoForInputDevice(EventArgs.InputDevice);

				if (!MappedUser || MappedUser->LocalPlayerIndex == INDEX_NONE)
				{
					if (LoginKeysForNewUser.Contains(EventArgs.Key))
					{
						// If we're in the middle of logging in just return true to ignore platform-specific input
						if (MappedUser && MappedUser->IsDoingLogin())
						{
							return true;
						}

						// Local multiplayer
						FEsotericUserInitializeParams NewParams = ParamsForLoginKey;
						NewParams.LocalPlayerIndex = NextLocalPlayerIndex;
						NewParams.PrimaryInputDevice = EventArgs.InputDevice;

						return TryToInitializeUser(NewParams);
					}
				}
			}
		}
	}

	if (WrappedInputKeyHandler.IsBound())
	{
		return WrappedInputKeyHandler.Execute(EventArgs);
	}

	return false;
}

static inline FText GetErrorText(const FOnlineErrorType& InOnlineError)
{
	return InOnlineError.GetErrorMessage();
}

void UEsotericUserSubsystem::HandleLoginForUserInitialize(const UEsotericUserInfo* UserInfo, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId,
	const TOptional<FOnlineErrorType>& InError, EEsotericUserOnlineContext Context, FEsotericUserInitializeParams Params)
{
	UGameInstance* GameInstance = GetGameInstance();
	check(GameInstance);
	FTimerManager& TimerManager = GameInstance->GetTimerManager();
	TOptional<FOnlineErrorType> Error = InError; // Copy so we can reset on handled errors

	UEsotericUserInfo* LocalUserInfo = ModifyInfo(UserInfo);
	UEsotericUserInfo* FirstUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(0));

	if (!ensure(LocalUserInfo && FirstUserInfo))
	{
		return;
	}

	// Check the hard platform/service ids
	RefreshLocalUserInfo(LocalUserInfo);

	FUniqueNetIdRepl FirstPlayerId = FirstUserInfo->GetNetId(EEsotericUserOnlineContext::PlatformOrDefault);

	// Check to see if we should make a guest after a login failure. Some platforms return success but reuse the first player's id, count this as a failure
	if (LocalUserInfo != FirstUserInfo && LocalUserInfo->bCanBeGuest && (NewStatus == ELoginStatusType::NotLoggedIn || NetId == FirstPlayerId))
	{
		NetId = (FUniqueNetIdRef)FUniqueNetIdString::Create(FString::Printf(TEXT("GuestPlayer%d"), LocalUserInfo->LocalPlayerIndex), NULL_SUBSYSTEM);
		LocalUserInfo->bIsGuest = true;
		NewStatus = ELoginStatusType::UsingLocalProfile;
		Error.Reset();
		UE_LOG(LogEsotericUser, Log, TEXT("HandleLoginForUserInitialize created guest id %s for local player %d"), *NetId.ToString(), LocalUserInfo->LocalPlayerIndex);
	}
	else
	{
		LocalUserInfo->bIsGuest = false;
	}

	ensure(LocalUserInfo->IsDoingLogin());

	if (Error.IsSet())
	{
		FText ErrorText = GetErrorText(Error.GetValue());
		TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEsotericUserSubsystem::HandleUserInitializeFailed, Params, ErrorText));
		return;
	}

	if (Context == EEsotericUserOnlineContext::Game)
	{
		LocalUserInfo->UpdateCachedNetId(NetId, EEsotericUserOnlineContext::Game);
	}
		
	ULocalPlayer* CurrentPlayer = GameInstance->GetLocalPlayerByIndex(LocalUserInfo->LocalPlayerIndex);
	if (!CurrentPlayer && Params.bCanCreateNewLocalPlayer)
	{
		FString ErrorString;
		CurrentPlayer = GameInstance->CreateLocalPlayer(LocalUserInfo->PlatformUser, ErrorString, true);

		if (!CurrentPlayer)
		{
			TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEsotericUserSubsystem::HandleUserInitializeFailed, Params, FText::AsCultureInvariant(ErrorString)));
			return;
		}
		ensure(GameInstance->GetLocalPlayerByIndex(LocalUserInfo->LocalPlayerIndex) == CurrentPlayer);
	}

	// Updates controller and net id if needed
	SetLocalPlayerUserInfo(CurrentPlayer, LocalUserInfo);

	// Set a delayed callback
	TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEsotericUserSubsystem::HandleUserInitializeSucceeded, Params));
}

void UEsotericUserSubsystem::HandleUserInitializeFailed(FEsotericUserInitializeParams Params, FText Error)
{
	UEsotericUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex));

	if (!LocalUserInfo)
	{
		// The user info was reset since this was scheduled
		return;
	}

	UE_LOG(LogEsotericUser, Warning, TEXT("TryToInitializeUser %d failed with error %s"), LocalUserInfo->LocalPlayerIndex, *Error.ToString());

	// If state is wrong, abort as we might have gotten canceled
	if (!ensure(LocalUserInfo->IsDoingLogin()))
	{
		return;
	}

	// If initial login failed or we ended up totally logged out, set to complete failure
	ELoginStatusType NewStatus = GetLocalUserLoginStatus(Params.PlatformUser, Params.OnlineContext);
	if (NewStatus == ELoginStatusType::NotLoggedIn || LocalUserInfo->InitializationState == EEsotericUserInitializationState::DoingInitialLogin)
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::FailedtoLogin;
	}
	else
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::LoggedInLocalOnly;
	}

	FText TitleText = NSLOCTEXT("EsotericUser", "LoginFailedTitle", "Login Failure");

	if (!Params.bSuppressLoginErrors)
	{
		SendSystemMessage(FEsotericUserTags::SystemMessage_Error_InitializeLocalPlayerFailed, TitleText, Error);
	}

	OnUserInitializeComplete.Broadcast(LocalUserInfo, false, Error, Params.RequestedPrivilege, Params.OnlineContext);
}

void UEsotericUserSubsystem::HandleUserInitializeSucceeded(FEsotericUserInitializeParams Params)
{
	UEsotericUserInfo* LocalUserInfo = ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex));

	if (!LocalUserInfo)
	{
		// The user info was reset since this was scheduled
		return;
	}

	// If state is wrong, abort as we might have gotten cancelled
	if (!ensure(LocalUserInfo->IsDoingLogin()))
	{
		return;
	}

	// Fix up state
	if (Params.RequestedPrivilege == EEsotericUserPrivilege::CanPlayOnline)
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::LoggedInOnline;
	}
	else
	{
		LocalUserInfo->InitializationState = EEsotericUserInitializationState::LoggedInLocalOnly;
	}

	ensure(LocalUserInfo->GetPrivilegeAvailability(Params.RequestedPrivilege) == EEsotericUserAvailability::NowAvailable);

	// Call callbacks
	Params.OnUserInitializeComplete.ExecuteIfBound(LocalUserInfo, true, FText(), Params.RequestedPrivilege, Params.OnlineContext);
	OnUserInitializeComplete.Broadcast(LocalUserInfo, true, FText(), Params.RequestedPrivilege, Params.OnlineContext);
}

bool UEsotericUserSubsystem::LoginLocalUser(const UEsotericUserInfo* UserInfo, EEsotericUserPrivilege RequestedPrivilege, EEsotericUserOnlineContext Context, FOnLocalUserLoginCompleteDelegate OnComplete)
{
	UEsotericUserInfo* LocalUserInfo = ModifyInfo(UserInfo);
	if (!ensure(UserInfo))
	{
		return false;
	}

	TSharedRef<FUserLoginRequest> NewRequest = MakeShared<FUserLoginRequest>(LocalUserInfo, RequestedPrivilege, Context, MoveTemp(OnComplete));
	ActiveLoginRequests.Add(NewRequest);

	// This will execute callback or start login process
	ProcessLoginRequest(NewRequest);

	return true;
}

void UEsotericUserSubsystem::ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request)
{
	// First, see if we've fully logged in
	UEsotericUserInfo* UserInfo = Request->UserInfo.Get();

	if (!UserInfo)
	{
		// User is gone, just delete this request
		ActiveLoginRequests.Remove(Request);

		return;
	}

	const FPlatformUserId PlatformUser = UserInfo->GetPlatformUserId();

	// If the platform user id is invalid because this is a guest, skip right to failure
	if (!IsRealPlatformUser(PlatformUser))
	{
		Request->Error = FOnlineError(NSLOCTEXT("EsotericUser", "InvalidPlatformUser", "Invalid Platform User"));
		// Remove from active array
		ActiveLoginRequests.Remove(Request);

		// Execute delegate if bound
		Request->Delegate.ExecuteIfBound(UserInfo, ELoginStatusType::NotLoggedIn, FUniqueNetIdRepl(), Request->Error, Request->DesiredContext);

		return;
	}

	// Figure out what context to process first
	if (Request->CurrentContext == EEsotericUserOnlineContext::Invalid)
	{
		// First start with platform context if this is a game login
		if (Request->DesiredContext == EEsotericUserOnlineContext::Game)
		{
			Request->CurrentContext = ResolveOnlineContext(EEsotericUserOnlineContext::PlatformOrDefault);
		}
		else
		{
			Request->CurrentContext = ResolveOnlineContext(Request->DesiredContext);
		}
	}

	ELoginStatusType CurrentStatus = GetLocalUserLoginStatus(PlatformUser, Request->CurrentContext);
	FUniqueNetIdRepl CurrentId = GetLocalUserNetId(PlatformUser, Request->CurrentContext);
	FOnlineContextCache* System = GetContextCache(Request->CurrentContext);

	if (!ensure(System))
	{
		return;
	}

	// Starting a new request
	if (Request->OverallLoginState == EEsotericUserAsyncTaskState::NotStarted)
	{
		Request->OverallLoginState = EEsotericUserAsyncTaskState::InProgress;
	}

	bool bHasRequiredStatus = (CurrentStatus == ELoginStatusType::LoggedIn);
	if (Request->DesiredPrivilege == EEsotericUserPrivilege::CanPlay)
	{
		// If this is not an online required login, allow local profile to count as fully logged in
		bHasRequiredStatus |= (CurrentStatus == ELoginStatusType::UsingLocalProfile);
	}

	// Check for overall success
	if (bHasRequiredStatus && CurrentId.IsValid())
	{
		// Stall if we're waiting for the login UI to close
		if (Request->LoginUIState == EEsotericUserAsyncTaskState::InProgress)
		{
			return;
		}

		Request->OverallLoginState = EEsotericUserAsyncTaskState::Done;
	}
	else
	{
		// Try using platform auth to login
		if (Request->TransferPlatformAuthState == EEsotericUserAsyncTaskState::NotStarted)
		{
			// We didn't start a login attempt, so set failure
			Request->TransferPlatformAuthState = EEsotericUserAsyncTaskState::Failed;
		}

		// Next check AutoLogin
		if (Request->AutoLoginState == EEsotericUserAsyncTaskState::NotStarted)
		{
			if (Request->TransferPlatformAuthState == EEsotericUserAsyncTaskState::Done || Request->TransferPlatformAuthState == EEsotericUserAsyncTaskState::Failed)
			{
				Request->AutoLoginState = EEsotericUserAsyncTaskState::InProgress;

				// Try an auto login with default credentials, this will work on many platforms
				if (AutoLogin(System, Request, PlatformUser))
				{
					return;
				}
				// We didn't start an autologin attempt, so set failure
				Request->AutoLoginState = EEsotericUserAsyncTaskState::Failed;
			}
		}

		// Next check login UI
		if (Request->LoginUIState == EEsotericUserAsyncTaskState::NotStarted)
		{
			if ((Request->TransferPlatformAuthState == EEsotericUserAsyncTaskState::Done || Request->TransferPlatformAuthState == EEsotericUserAsyncTaskState::Failed)
				&& (Request->AutoLoginState == EEsotericUserAsyncTaskState::Done || Request->AutoLoginState == EEsotericUserAsyncTaskState::Failed))
			{
				Request->LoginUIState = EEsotericUserAsyncTaskState::InProgress;

				if (ShowLoginUI(System, Request, PlatformUser))
				{
					return;
				}
				// We didn't show a UI, so set failure
				Request->LoginUIState = EEsotericUserAsyncTaskState::Failed;
			}
		}
	}

	// Check for overall failure
	if (Request->LoginUIState == EEsotericUserAsyncTaskState::Failed &&
		Request->AutoLoginState == EEsotericUserAsyncTaskState::Failed &&
		Request->TransferPlatformAuthState == EEsotericUserAsyncTaskState::Failed)
	{
		Request->OverallLoginState = EEsotericUserAsyncTaskState::Failed;
	}
	else if (Request->OverallLoginState == EEsotericUserAsyncTaskState::InProgress &&
		Request->LoginUIState != EEsotericUserAsyncTaskState::InProgress &&
		Request->AutoLoginState != EEsotericUserAsyncTaskState::InProgress &&
		Request->TransferPlatformAuthState != EEsotericUserAsyncTaskState::InProgress)
	{
		// If none of the substates are still in progress but we haven't successfully logged in, mark this as a failure to avoid stalling forever
		Request->OverallLoginState = EEsotericUserAsyncTaskState::Failed;
	}

	if (Request->OverallLoginState == EEsotericUserAsyncTaskState::Done)
	{
		// Do the permissions check if needed
		if (Request->PrivilegeCheckState == EEsotericUserAsyncTaskState::NotStarted)
		{
			Request->PrivilegeCheckState = EEsotericUserAsyncTaskState::InProgress;

			EEsotericUserPrivilegeResult CachedResult = UserInfo->GetCachedPrivilegeResult(Request->DesiredPrivilege, Request->CurrentContext);
			if (CachedResult == EEsotericUserPrivilegeResult::Available)
			{
				// Use cached success value
				Request->PrivilegeCheckState = EEsotericUserAsyncTaskState::Done;
			}
			else
			{
				if (QueryUserPrivilege(System, Request, PlatformUser))
				{
					return;
				}
			}
		}

		if (Request->PrivilegeCheckState == EEsotericUserAsyncTaskState::Failed)
		{
			// Count a privilege failure as a login failure
			Request->OverallLoginState = EEsotericUserAsyncTaskState::Failed;
		}
		else if (Request->PrivilegeCheckState == EEsotericUserAsyncTaskState::Done)
		{
			// If platform context done but still need to do service context, do that next
			EEsotericUserOnlineContext ResolvedDesiredContext = ResolveOnlineContext(Request->DesiredContext);

			if (Request->OverallLoginState == EEsotericUserAsyncTaskState::Done && Request->CurrentContext != ResolvedDesiredContext)
			{
				Request->CurrentContext = ResolvedDesiredContext;
				Request->OverallLoginState = EEsotericUserAsyncTaskState::NotStarted;
				Request->PrivilegeCheckState = EEsotericUserAsyncTaskState::NotStarted;
				Request->TransferPlatformAuthState = EEsotericUserAsyncTaskState::NotStarted;
				Request->AutoLoginState = EEsotericUserAsyncTaskState::NotStarted;
				Request->LoginUIState = EEsotericUserAsyncTaskState::NotStarted;

				// Reprocess and immediately return
				ProcessLoginRequest(Request);
				return;
			}
		}
	}

	if (Request->PrivilegeCheckState == EEsotericUserAsyncTaskState::InProgress)
	{
		// Stall to wait for it to finish
		return;
	}

	// If done, remove and do callback
	if (Request->OverallLoginState == EEsotericUserAsyncTaskState::Done || Request->OverallLoginState == EEsotericUserAsyncTaskState::Failed)
	{
		// Skip if this already happened in a nested function
		if (ActiveLoginRequests.Contains(Request))
		{
			// Add a generic error if none is set
			if (Request->OverallLoginState == EEsotericUserAsyncTaskState::Failed && !Request->Error.IsSet())
			{
				Request->Error = FOnlineError(NSLOCTEXT("EsotericUser", "FailedToRequest", "Failed to Request Login"));
			}

			// Remove from active array
			ActiveLoginRequests.Remove(Request);

			// Execute delegate if bound
			Request->Delegate.ExecuteIfBound(UserInfo, CurrentStatus, CurrentId, Request->Error, Request->DesiredContext);
		}
	}
}

void UEsotericUserSubsystem::HandleUserLoginCompleted(int32 PlatformUserIndex, bool bWasSuccessful,
	const FUniqueNetId& NetId, const FString& Error, EEsotericUserOnlineContext Context)
{
	FPlatformUserId PlatformUser = GetPlatformUserIdForIndex(PlatformUserIndex);
	ELoginStatusType NewStatus = GetLocalUserLoginStatus(PlatformUser, Context);
	FUniqueNetIdRepl NewId = FUniqueNetIdRepl(NetId);
	UE_LOG(LogEsotericUser, Log, TEXT("Player login Completed - System:%s, UserIdx:%d, Successful:%d, NewStatus:%s, NewId:%s, ErrorIfAny:%s"),
		*GetOnlineSubsystemName(Context).ToString(),
		PlatformUserIndex,
		(int32)bWasSuccessful,
		ELoginStatus::ToString(NewStatus),
		*NewId.ToString(),
		*Error);

	// Update any waiting login requests
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		UEsotericUserInfo* UserInfo = Request->UserInfo.Get();

		if (!UserInfo)
		{
			// User is gone, just delete this request
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		if (UserInfo->PlatformUser == PlatformUser && Request->CurrentContext == Context)
		{
			// On some platforms this gets called from the login UI with a failure
			if (Request->AutoLoginState == EEsotericUserAsyncTaskState::InProgress)
			{
				Request->AutoLoginState = bWasSuccessful ? EEsotericUserAsyncTaskState::Done : EEsotericUserAsyncTaskState::Failed;
			}

			if (!bWasSuccessful)
			{
				Request->Error = FOnlineError(FText::FromString(Error));
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UEsotericUserSubsystem::HandleOnLoginUIClosed(TSharedPtr<const FUniqueNetId> LoggedInNetId,
                                                   const int PlatformUserIndex, const FOnlineError& Error, EEsotericUserOnlineContext Context)
{
	FPlatformUserId PlatformUser = GetPlatformUserIdForIndex(PlatformUserIndex);

	// Update any waiting login requests
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		UEsotericUserInfo* UserInfo = Request->UserInfo.Get();

		if (!UserInfo)
		{
			// User is gone, just delete this request
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		// Look for first user trying to log in on this context
		if (Request->CurrentContext == Context && Request->LoginUIState == EEsotericUserAsyncTaskState::InProgress)
		{
			if (LoggedInNetId.IsValid() && LoggedInNetId->IsValid() && Error.WasSuccessful())
			{
				// The platform user id that actually logged in may not be the same one who requested the UI,
				// so swap it if the returned id is actually valid
				if (UserInfo->PlatformUser != PlatformUser && PlatformUser != PLATFORMUSERID_NONE)
				{
					UserInfo->PlatformUser = PlatformUser;
				}

				Request->LoginUIState = EEsotericUserAsyncTaskState::Done;
				Request->Error.Reset();
			}
			else
			{
				Request->LoginUIState = EEsotericUserAsyncTaskState::Failed;
				Request->Error = Error;
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UEsotericUserSubsystem::HandleCheckPrivilegesComplete(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege,
	uint32 PrivilegeResults, EEsotericUserPrivilege UserPrivilege,
	TWeakObjectPtr<UEsotericUserInfo> EsotericUserInfo, EEsotericUserOnlineContext Context)
{
	// Only handle if user still exists
	UEsotericUserInfo* UserInfo = EsotericUserInfo.Get();

	if (!UserInfo)
	{
		return;
	}

	EEsotericUserPrivilegeResult UserResult = ConvertOSSPrivilegeResult(Privilege, PrivilegeResults);

	// Update the user cached value
	UpdateUserPrivilegeResult(UserInfo, UserPrivilege, UserResult, Context);

	FOnlineContextCache* ContextCache = GetContextCache(Context);
	check(ContextCache);

	// If this returns disconnected, update the connection status
	if (UserResult == EEsotericUserPrivilegeResult::NetworkConnectionUnavailable)
	{
		ContextCache->CurrentConnectionStatus = EOnlineServerConnectionStatus::NoNetworkConnection;
	}
	else if (UserResult == EEsotericUserPrivilegeResult::Available && UserPrivilege == EEsotericUserPrivilege::CanPlayOnline)
	{
		if (ContextCache->CurrentConnectionStatus == EOnlineServerConnectionStatus::NoNetworkConnection)
		{
			ContextCache->CurrentConnectionStatus = EOnlineServerConnectionStatus::Normal;
		}
	}
		
	// See if a login request is waiting on this
	TArray<TSharedRef<FUserLoginRequest>> RequestsCopy = ActiveLoginRequests;
	for (TSharedRef<FUserLoginRequest>& Request : RequestsCopy)
	{
		if (Request->UserInfo.Get() == UserInfo && Request->CurrentContext == Context && Request->DesiredPrivilege == UserPrivilege && Request->PrivilegeCheckState == EEsotericUserAsyncTaskState::InProgress)
		{
			if (UserResult == EEsotericUserPrivilegeResult::Available)
			{
				Request->PrivilegeCheckState = EEsotericUserAsyncTaskState::Done;
			}
			else
			{
				Request->PrivilegeCheckState = EEsotericUserAsyncTaskState::Failed;

				// Forms strings in english like "(The user is not allowed) to (play the game)"
				Request->Error = FOnlineError(FText::Format(NSLOCTEXT("EsotericUser", "PrivilegeFailureFormat", "{0} to {1}"), GetPrivilegeResultDescription(UserResult), GetPrivilegeDescription(UserPrivilege)));
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UEsotericUserSubsystem::RefreshLocalUserInfo(UEsotericUserInfo* UserInfo)
{
	if (ensure(UserInfo))
	{
		// Always update default
		UserInfo->UpdateCachedNetId(GetLocalUserNetId(UserInfo->PlatformUser, EEsotericUserOnlineContext::Default), EEsotericUserOnlineContext::Default);

		if (HasSeparatePlatformContext())
		{
			// Also update platform
			UserInfo->UpdateCachedNetId(GetLocalUserNetId(UserInfo->PlatformUser, EEsotericUserOnlineContext::Platform), EEsotericUserOnlineContext::Platform);
		}
	}
}

void UEsotericUserSubsystem::HandleChangedAvailability(UEsotericUserInfo* UserInfo, EEsotericUserPrivilege Privilege,
	EEsotericUserAvailability OldAvailability)
{
	EEsotericUserAvailability NewAvailability = UserInfo->GetPrivilegeAvailability(Privilege);

	if (OldAvailability != NewAvailability)
	{
		OnUserPrivilegeChanged.Broadcast(UserInfo, Privilege, OldAvailability, NewAvailability);
	}
}

void UEsotericUserSubsystem::UpdateUserPrivilegeResult(UEsotericUserInfo* UserInfo, EEsotericUserPrivilege Privilege,
	EEsotericUserPrivilegeResult Result, EEsotericUserOnlineContext Context)
{
	check(UserInfo);
	
	EEsotericUserAvailability OldAvailability = UserInfo->GetPrivilegeAvailability(Privilege);

	UserInfo->UpdateCachedPrivilegeResult(Privilege, Result, Context);

	HandleChangedAvailability(UserInfo, Privilege, OldAvailability);
}

EEsotericUserPrivilege UEsotericUserSubsystem::ConvertOSSPrivilege(EUserPrivileges::Type Privilege) const
{
	switch (Privilege)
	{
	case EUserPrivileges::CanPlay:
		return EEsotericUserPrivilege::CanPlay;
	case EUserPrivileges::CanPlayOnline:
		return EEsotericUserPrivilege::CanPlayOnline;
	case EUserPrivileges::CanCommunicateOnline:
		return EEsotericUserPrivilege::CanCommunicateViaTextOnline; // No good thing to do here, just mapping to text.
	case EUserPrivileges::CanUseUserGeneratedContent:
		return EEsotericUserPrivilege::CanUseUserGeneratedContent;
	case EUserPrivileges::CanUserCrossPlay:
		return EEsotericUserPrivilege::CanUseCrossPlay;
	default:
		return EEsotericUserPrivilege::Invalid_Count;
	}
}

EUserPrivileges::Type UEsotericUserSubsystem::ConvertOSSPrivilege(EEsotericUserPrivilege Privilege) const
{
	switch (Privilege)
	{
	case EEsotericUserPrivilege::CanPlay:
		return EUserPrivileges::CanPlay;
	case EEsotericUserPrivilege::CanPlayOnline:
		return EUserPrivileges::CanPlayOnline;
	case EEsotericUserPrivilege::CanCommunicateViaTextOnline:
	case EEsotericUserPrivilege::CanCommunicateViaVoiceOnline:
		return EUserPrivileges::CanCommunicateOnline;
	case EEsotericUserPrivilege::CanUseUserGeneratedContent:
		return EUserPrivileges::CanUseUserGeneratedContent;
	case EEsotericUserPrivilege::CanUseCrossPlay:
		return EUserPrivileges::CanUserCrossPlay;
	default:
		// No failure type, return CanPlay
			return EUserPrivileges::CanPlay;
	}
}

EEsotericUserPrivilegeResult UEsotericUserSubsystem::ConvertOSSPrivilegeResult(EUserPrivileges::Type Privilege,
	uint32 Results) const
{
	// The V1 results enum is a bitfield where each platform behaves a bit differently
	if (Results == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		return EEsotericUserPrivilegeResult::Available;
	}
	if ((Results & (uint32)IOnlineIdentity::EPrivilegeResults::UserNotFound) || (Results & (uint32)IOnlineIdentity::EPrivilegeResults::UserNotLoggedIn))
	{
		return EEsotericUserPrivilegeResult::UserNotLoggedIn;
	}
	if ((Results & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable) || (Results & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate))
	{
		return EEsotericUserPrivilegeResult::VersionOutdated;
	}
	if (Results & (uint32)IOnlineIdentity::EPrivilegeResults::AgeRestrictionFailure)
	{
		return EEsotericUserPrivilegeResult::AgeRestricted;
	}
	if (Results & (uint32)IOnlineIdentity::EPrivilegeResults::AccountTypeFailure)
	{
		return EEsotericUserPrivilegeResult::AccountTypeRestricted;
	}
	if (Results & (uint32)IOnlineIdentity::EPrivilegeResults::NetworkConnectionUnavailable)
	{
		return EEsotericUserPrivilegeResult::NetworkConnectionUnavailable;
	}

	// Bucket other account failures together
	uint32 AccountUseFailures = (uint32)IOnlineIdentity::EPrivilegeResults::OnlinePlayRestricted 
		| (uint32)IOnlineIdentity::EPrivilegeResults::UGCRestriction 
		| (uint32)IOnlineIdentity::EPrivilegeResults::ChatRestriction;

	if (Results & AccountUseFailures)
	{
		return EEsotericUserPrivilegeResult::AccountUseRestricted;
	}

	// If you can't play at all, this is a license failure
	if (Privilege == EUserPrivileges::CanPlay)
	{
		return EEsotericUserPrivilegeResult::LicenseInvalid;
	}

	// Unknown reason
	return EEsotericUserPrivilegeResult::PlatformFailure;
}

FString UEsotericUserSubsystem::PlatformUserIdToString(FPlatformUserId UserId)
{
	if (UserId == PLATFORMUSERID_NONE)
	{
		return TEXT("None");
	}
	else
	{
		return FString::Printf(TEXT("%d"), UserId.GetInternalId());
	}
}

FString UEsotericUserSubsystem::EEsotericUserOnlineContextToString(EEsotericUserOnlineContext Context)
{
	switch (Context)
	{
	case EEsotericUserOnlineContext::Game:
		return TEXT("Game");
	case EEsotericUserOnlineContext::Default:
		return TEXT("Default");
	case EEsotericUserOnlineContext::Service:
		return TEXT("Service");
	case EEsotericUserOnlineContext::ServiceOrDefault:
		return TEXT("Service/Default");
	case EEsotericUserOnlineContext::Platform:
		return TEXT("Platform");
	case EEsotericUserOnlineContext::PlatformOrDefault:
		return TEXT("Platform/Default");
	default:
		return TEXT("Invalid");
	}
}

FText UEsotericUserSubsystem::GetPrivilegeDescription(EEsotericUserPrivilege Privilege) const
{
	switch (Privilege)
	{
	case EEsotericUserPrivilege::CanPlay:
		return NSLOCTEXT("EsotericUser", "PrivilegeCanPlay", "play the game");
	case EEsotericUserPrivilege::CanPlayOnline:
		return NSLOCTEXT("EsotericUser", "PrivilegeCanPlayOnline", "play online");
	case EEsotericUserPrivilege::CanCommunicateViaTextOnline:
		return NSLOCTEXT("EsotericUser", "PrivilegeCanCommunicateViaTextOnline", "communicate with text");
	case EEsotericUserPrivilege::CanCommunicateViaVoiceOnline:
		return NSLOCTEXT("EsotericUser", "PrivilegeCanCommunicateViaVoiceOnline", "communicate with voice");
	case EEsotericUserPrivilege::CanUseUserGeneratedContent:
		return NSLOCTEXT("EsotericUser", "PrivilegeCanUseUserGeneratedContent", "access user content");
	case EEsotericUserPrivilege::CanUseCrossPlay:
		return NSLOCTEXT("EsotericUser", "PrivilegeCanUseCrossPlay", "play with other platforms");
	default:
		return NSLOCTEXT("EsotericUser", "PrivilegeInvalid", "");
	}
}

FText UEsotericUserSubsystem::GetPrivilegeResultDescription(EEsotericUserPrivilegeResult Result) const
{
	// TODO these strings might have cert requirements we need to override per console
	switch (Result)
	{
	case EEsotericUserPrivilegeResult::Unknown:
		return NSLOCTEXT("EsotericUser", "ResultUnknown", "Unknown if the user is allowed");
	case EEsotericUserPrivilegeResult::Available:
		return NSLOCTEXT("EsotericUser", "ResultAvailable", "The user is allowed");
	case EEsotericUserPrivilegeResult::UserNotLoggedIn:
		return NSLOCTEXT("EsotericUser", "ResultUserNotLoggedIn", "The user must login");
	case EEsotericUserPrivilegeResult::LicenseInvalid:
		return NSLOCTEXT("EsotericUser", "ResultLicenseInvalid", "A valid game license is required");
	case EEsotericUserPrivilegeResult::VersionOutdated:
		return NSLOCTEXT("EsotericUser", "VersionOutdated", "The game or hardware needs to be updated");
	case EEsotericUserPrivilegeResult::NetworkConnectionUnavailable:
		return NSLOCTEXT("EsotericUser", "ResultNetworkConnectionUnavailable", "A network connection is required");
	case EEsotericUserPrivilegeResult::AgeRestricted:
		return NSLOCTEXT("EsotericUser", "ResultAgeRestricted", "This age restricted account is not allowed");
	case EEsotericUserPrivilegeResult::AccountTypeRestricted:
		return NSLOCTEXT("EsotericUser", "ResultAccountTypeRestricted", "This account type does not have access");
	case EEsotericUserPrivilegeResult::AccountUseRestricted:
		return NSLOCTEXT("EsotericUser", "ResultAccountUseRestricted", "This account is not allowed");
	case EEsotericUserPrivilegeResult::PlatformFailure:
		return NSLOCTEXT("EsotericUser", "ResultPlatformFailure", "Not allowed");
	default:
		return NSLOCTEXT("EsotericUser", "ResultInvalid", "");

	}
}

const UEsotericUserSubsystem::FOnlineContextCache* UEsotericUserSubsystem::GetContextCache(EEsotericUserOnlineContext Context) const
{
	return const_cast<UEsotericUserSubsystem*>(this)->GetContextCache(Context);
}

UEsotericUserSubsystem::FOnlineContextCache* UEsotericUserSubsystem::GetContextCache(EEsotericUserOnlineContext Context)
{
	switch (Context)
	{
	case EEsotericUserOnlineContext::Game:
	case EEsotericUserOnlineContext::Default:
		return DefaultContextInternal;

	case EEsotericUserOnlineContext::Service:
		return ServiceContextInternal;
	case EEsotericUserOnlineContext::ServiceOrDefault:
		return ServiceContextInternal ? ServiceContextInternal : DefaultContextInternal;

	case EEsotericUserOnlineContext::Platform:
		return PlatformContextInternal;
	case EEsotericUserOnlineContext::PlatformOrDefault:
		return PlatformContextInternal ? PlatformContextInternal : DefaultContextInternal;
	}

	return nullptr;
}

EEsotericUserOnlineContext UEsotericUserSubsystem::ResolveOnlineContext(EEsotericUserOnlineContext Context) const
{
	switch (Context)
	{
	case EEsotericUserOnlineContext::Game:
	case EEsotericUserOnlineContext::Default:
		return EEsotericUserOnlineContext::Default;

	case EEsotericUserOnlineContext::Service:
		return ServiceContextInternal ? EEsotericUserOnlineContext::Service : EEsotericUserOnlineContext::Invalid;
	case EEsotericUserOnlineContext::ServiceOrDefault:
		return ServiceContextInternal ? EEsotericUserOnlineContext::Service : EEsotericUserOnlineContext::Default;

	case EEsotericUserOnlineContext::Platform:
		return PlatformContextInternal ? EEsotericUserOnlineContext::Platform : EEsotericUserOnlineContext::Invalid;
	case EEsotericUserOnlineContext::PlatformOrDefault:
		return PlatformContextInternal ? EEsotericUserOnlineContext::Platform : EEsotericUserOnlineContext::Default;
	}

	return  EEsotericUserOnlineContext::Invalid;
}

bool UEsotericUserSubsystem::HasSeparatePlatformContext() const
{
	EEsotericUserOnlineContext ServiceType = ResolveOnlineContext(EEsotericUserOnlineContext::ServiceOrDefault);
	EEsotericUserOnlineContext PlatformType = ResolveOnlineContext(EEsotericUserOnlineContext::PlatformOrDefault);

	if (ServiceType != PlatformType)
	{
		return true;
	}
	return false;
}

void UEsotericUserSubsystem::SetLocalPlayerUserInfo(ULocalPlayer* LocalPlayer, const UEsotericUserInfo* UserInfo)
{
	if (!bIsDedicatedServer && ensure(LocalPlayer && UserInfo))
	{
		LocalPlayer->SetPlatformUserId(UserInfo->GetPlatformUserId());

		FUniqueNetIdRepl NetId = UserInfo->GetNetId(EEsotericUserOnlineContext::Game);
		LocalPlayer->SetCachedUniqueNetId(NetId);

		// Also update player state if possible
		APlayerController* PlayerController = LocalPlayer->GetPlayerController(nullptr);
		if (PlayerController && PlayerController->PlayerState)
		{
			PlayerController->PlayerState->SetUniqueId(NetId);
		}
	}
}

const UEsotericUserInfo* UEsotericUserSubsystem::GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const
{
	TObjectPtr<UEsotericUserInfo> const* Found = LocalUserInfos.Find(LocalPlayerIndex);
	if (Found)
	{
		return *Found;
	}
	return nullptr;
}

const UEsotericUserInfo* UEsotericUserSubsystem::GetUserInfoForPlatformUserIndex(int32 PlatformUserIndex) const
{
	FPlatformUserId PlatformUser = GetPlatformUserIdForIndex(PlatformUserIndex);
	return GetUserInfoForPlatformUser(PlatformUser);
}

const UEsotericUserInfo* UEsotericUserSubsystem::GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return nullptr;
	}

	for (TPair<int32, UEsotericUserInfo*> Pair : LocalUserInfos)
	{
		// Don't include guest users in this check
		if (ensure(Pair.Value) && Pair.Value->PlatformUser == PlatformUser && !Pair.Value->bIsGuest)
		{
			return Pair.Value;
		}
	}

	return nullptr;
}

const UEsotericUserInfo* UEsotericUserSubsystem::GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const
{
	if (!NetId.IsValid())
	{
		// TODO do we need to handle pre-login case on mobile platforms where netID is invalid?
		return nullptr;
	}

	for (TPair<int32, UEsotericUserInfo*> UserPair : LocalUserInfos)
	{
		if (ensure(UserPair.Value))
		{
			for (const TPair<EEsotericUserOnlineContext, UEsotericUserInfo::FCachedData>& CachedPair : UserPair.Value->CachedDataMap)
			{
				if (NetId == CachedPair.Value.CachedNetId)
				{
					return UserPair.Value;
				}
			}
		}
	}

	return nullptr;
}

const UEsotericUserInfo* UEsotericUserSubsystem::GetUserInfoForControllerId(int32 ControllerId) const
{
	FPlatformUserId PlatformUser;
	FInputDeviceId IgnoreDevice;

	IPlatformInputDeviceMapper::Get().RemapControllerIdToPlatformUserAndDevice(ControllerId, PlatformUser, IgnoreDevice);

	return GetUserInfoForPlatformUser(PlatformUser);
}

const UEsotericUserInfo* UEsotericUserSubsystem::GetUserInfoForInputDevice(FInputDeviceId InputDevice) const
{
	FPlatformUserId PlatformUser = GetPlatformUserIdForInputDevice(InputDevice);
	return GetUserInfoForPlatformUser(PlatformUser);
}

bool UEsotericUserSubsystem::IsRealPlatformUserIndex(int32 PlatformUserIndex) const
{
	if (PlatformUserIndex < 0)
	{
		return false;
	}

	if (PlatformUserIndex >= MAX_LOCAL_PLAYERS)
	{
		// Check against OSS count
		return false;
	}

	if (PlatformUserIndex > 0 && GetTraitTags().HasTag(FEsotericUserTags::Platform_Trait_SingleOnlineUser))
	{
		return false;
	}

	return true;
}

bool UEsotericUserSubsystem::IsRealPlatformUser(FPlatformUserId PlatformUser) const
{
	// Validation is done at conversion/allocation time so trust the type
	if (!PlatformUser.IsValid())
	{
		return false;
	}

	// TODO: Validate against OSS or input mapper somehow

	if (GetTraitTags().HasTag(FEsotericUserTags::Platform_Trait_SingleOnlineUser))
	{
		// Only the default user is supports online functionality 
		if (PlatformUser != IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser())
		{
			return false;
		}
	}

	return true;
}

FPlatformUserId UEsotericUserSubsystem::GetPlatformUserIdForIndex(int32 PlatformUserIndex) const
{
	return IPlatformInputDeviceMapper::Get().GetPlatformUserForUserIndex(PlatformUserIndex);
}

int32 UEsotericUserSubsystem::GetPlatformUserIndexForId(FPlatformUserId PlatformUser) const
{
	return IPlatformInputDeviceMapper::Get().GetUserIndexForPlatformUser(PlatformUser);
}

FPlatformUserId UEsotericUserSubsystem::GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const
{
	return IPlatformInputDeviceMapper::Get().GetUserForInputDevice(InputDevice);
}

FInputDeviceId UEsotericUserSubsystem::GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const
{
	return IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(PlatformUser);
}

void UEsotericUserSubsystem::SetTraitTags(const FGameplayTagContainer& InTags)
{
	CachedTraitTags = InTags;
}


bool UEsotericUserSubsystem::ShouldWaitForStartInput() const
{
	// By default, don't wait for input if this is a single user platform
	return !HasTraitTag(FEsotericUserTags::Platform_Trait_SingleOnlineUser.GetTag());
}

void UEsotericUserSubsystem::HandleIdentityLoginStatusChanged(int32 PlatformUserIndex, ELoginStatus::Type OldStatus,
	ELoginStatus::Type NewStatus, const FUniqueNetId& NewId, EEsotericUserOnlineContext Context)
{
	UE_LOG(LogEsotericUser, Log, TEXT("Player login status changed - System:%s, UserIdx:%d, OldStatus:%s, NewStatus:%s, NewId:%s"),
	*GetOnlineSubsystemName(Context).ToString(),
	PlatformUserIndex,
	ELoginStatus::ToString(OldStatus),
	ELoginStatus::ToString(NewStatus),
	*NewId.ToString());

	if (NewStatus == ELoginStatus::NotLoggedIn && OldStatus != ELoginStatus::NotLoggedIn)
	{
		FPlatformUserId PlatformUser = GetPlatformUserIdForIndex(PlatformUserIndex);
		LogOutLocalUser(PlatformUser);
	}
}

void UEsotericUserSubsystem::HandleControllerPairingChanged(int32 PlatformUserIndex,
	FControllerPairingChangedUserInfo PreviousUser, FControllerPairingChangedUserInfo NewUser)
{
	UE_LOG(LogEsotericUser, Log, TEXT("Player controller pairing changed - UserIdx:%d, PreviousUser:%s, NewUser:%s"),
		PlatformUserIndex,
		*ToDebugString(PreviousUser),
		*ToDebugString(NewUser));

	UGameInstance* GameInstance = GetGameInstance();
	FPlatformUserId PlatformUser = GetPlatformUserIdForIndex(PlatformUserIndex);
	ULocalPlayer* ControlledLocalPlayer = GameInstance->FindLocalPlayerFromPlatformUserId(PlatformUser);
	ULocalPlayer* NewLocalPlayer = GameInstance->FindLocalPlayerFromUniqueNetId(NewUser.User);
	const UEsotericUserInfo* NewUserInfo = GetUserInfoForUniqueNetId(FUniqueNetIdRepl(NewUser.User));
	const UEsotericUserInfo* PreviousUserInfo = GetUserInfoForUniqueNetId(FUniqueNetIdRepl(PreviousUser.User));

	// See if we think this is already bound to an existing player	
	if (PreviousUser.ControllersRemaining == 0 && PreviousUserInfo && PreviousUserInfo != NewUserInfo)
	{
		// This means that the user deliberately logged out using a platform interface
		if (IsRealPlatformUser(PlatformUser))
		{
			LogOutLocalUser(PlatformUser);
		}
	}

	if (ControlledLocalPlayer && ControlledLocalPlayer != NewLocalPlayer)
	{
		// TODO Currently the platforms that call this delegate do not really handle swapping controller IDs
		// SetLocalPlayerUserIndex(ControlledLocalPlayer, -1);
	}
}

void UEsotericUserSubsystem::HandleNetworkConnectionStatusChanged(const FString& ServiceName,
	EOnlineServerConnectionStatus::Type LastConnectionStatus, EOnlineServerConnectionStatus::Type ConnectionStatus,
	EEsotericUserOnlineContext Context)
{
	UE_LOG(LogEsotericUser, Log, TEXT("HandleNetworkConnectionStatusChanged(ServiceName: %s, LastStatus: %s, ConnectionStatus: %s)"),
		*ServiceName,
		EOnlineServerConnectionStatus::ToString(LastConnectionStatus),
		EOnlineServerConnectionStatus::ToString(ConnectionStatus));

	// Cache old availablity for current users
	TMap<UEsotericUserInfo*, EEsotericUserAvailability> AvailabilityMap;

	for (TPair<int32, UEsotericUserInfo*> Pair : LocalUserInfos)
	{
		AvailabilityMap.Add(Pair.Value, Pair.Value->GetPrivilegeAvailability(EEsotericUserPrivilege::CanPlayOnline));
	}

	FOnlineContextCache* System = GetContextCache(Context);
	if (ensure(System))
	{
		// Service name is normally the same as the OSS name, but not necessarily on all platforms
		System->CurrentConnectionStatus = ConnectionStatus;
	}

	for (TPair<UEsotericUserInfo*, EEsotericUserAvailability> Pair : AvailabilityMap)
	{
		// Notify other systems when someone goes online/offline
		HandleChangedAvailability(Pair.Key, EEsotericUserPrivilege::CanPlayOnline, Pair.Value);
	}
}

void UEsotericUserSubsystem::HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState,
	FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	FString InputDeviceIDString = FString::Printf(TEXT("%d"), InputDeviceId.GetId());
	const bool bIsConnected = NewConnectionState == EInputDeviceConnectionState::Connected;

	// TODO Implement for platforms that support this
}











