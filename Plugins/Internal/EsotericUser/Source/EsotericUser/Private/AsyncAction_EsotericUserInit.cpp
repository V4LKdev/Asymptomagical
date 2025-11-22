// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncAction_EsotericUserInit.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_EsotericUserInit)

UAsyncAction_EsotericUserInit* UAsyncAction_EsotericUserInit::InitializeForLocalPlay(UEsotericUserSubsystem* Target,
	int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
	if (!PrimaryInputDevice.IsValid())
	{
		// Set to default device
		PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	}

	UAsyncAction_EsotericUserInit* Action = NewObject<UAsyncAction_EsotericUserInit>();

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;
		
		Action->Params.RequestedPrivilege = EEsotericUserPrivilege::CanPlay;
		Action->Params.LocalPlayerIndex = LocalPlayerIndex;
		Action->Params.PrimaryInputDevice = PrimaryInputDevice;
		Action->Params.bCanUseGuestLogin = bCanUseGuestLogin;
		Action->Params.bCanCreateNewLocalPlayer = true;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}

UAsyncAction_EsotericUserInit* UAsyncAction_EsotericUserInit::LoginForOnlinePlay(UEsotericUserSubsystem* Target,
	int32 LocalPlayerIndex)
{
	UAsyncAction_EsotericUserInit* Action = NewObject<UAsyncAction_EsotericUserInit>();

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;
		
		Action->Params.RequestedPrivilege = EEsotericUserPrivilege::CanPlayOnline;
		Action->Params.LocalPlayerIndex = LocalPlayerIndex;
		Action->Params.bCanCreateNewLocalPlayer = false;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}

void UAsyncAction_EsotericUserInit::HandleFailure()
{
	const UEsotericUserInfo* UserInfo = nullptr;
	if (Subsystem.IsValid())
	{
		UserInfo = Subsystem->GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex);
	}
	HandleInitializationComplete(UserInfo, false, NSLOCTEXT("EsotericUser", "LoginFailedEarly", "Unable to start login process"), Params.RequestedPrivilege, Params.OnlineContext);
}

void UAsyncAction_EsotericUserInit::HandleInitializationComplete(const UEsotericUserInfo* UserInfo, bool bSuccess,
	FText Error, EEsotericUserPrivilege RequestedPrivilege, EEsotericUserOnlineContext OnlineContext)
{
	if (ShouldBroadcastDelegates())
	{
		OnInitializationComplete.Broadcast(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);
	}

	SetReadyToDestroy();
}

void UAsyncAction_EsotericUserInit::Activate()
{
	if (Subsystem.IsValid())
	{
		Params.OnUserInitializeComplete.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UAsyncAction_EsotericUserInit, HandleInitializationComplete));
		bool bSuccess = Subsystem->TryToInitializeUser(Params);

		if (!bSuccess)
		{
			// Call failure next frame
			FTimerManager* TimerManager = GetTimerManager();
			
			if (TimerManager)
			{
				TimerManager->SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UAsyncAction_EsotericUserInit::HandleFailure));
			}
		}
	}
	else
	{
		SetReadyToDestroy();
	}	
}
