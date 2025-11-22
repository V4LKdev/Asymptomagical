// Copyright 2024 Nic, Vlad, Alex


#include "AsymGameViewportClient.h"

bool UAsymGameViewportClient::bAllowWindowClosing = true;

UAsymGameViewportClient::UAsymGameViewportClient() : Super()
{
	OnRerouteInput().BindUObject(this, &ThisClass::HandleRerouteInput);
	OnRerouteAxis().BindUObject(this, &ThisClass::HandleRerouteAxis);
	OnRerouteTouch().BindUObject(this, &ThisClass::HandleRerouteTouch);
}

bool UAsymGameViewportClient::WindowCloseRequested()
{
	if (bAllowWindowClosing)
	{
		return true;
	}
	UserWindowCloseRequested.Execute();
	return false;
}

void UAsymGameViewportClient::SetAllowWindowClose(bool bAllowWindowClose)
{
	bAllowWindowClosing = bAllowWindowClose;
}

bool UAsymGameViewportClient::GetAllowWindowClose()
{
	return bAllowWindowClosing;
}

void UAsymGameViewportClient::BindOnUserWindowCloseRequested(FOnUserWindowCloseRequested OnUserWindowCloseRequested)
{
	UserWindowCloseRequested = OnUserWindowCloseRequested;
}
