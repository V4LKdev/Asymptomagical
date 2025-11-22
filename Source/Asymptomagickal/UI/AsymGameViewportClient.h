// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "CommonGameViewportClient.h"
#include "AsymGameViewportClient.generated.h"


/**
 * Catches the WindowCloseRequest and handles according to the set rule
 */
DECLARE_DYNAMIC_DELEGATE(FOnUserWindowCloseRequested);

static FOnUserWindowCloseRequested UserWindowCloseRequested;


UCLASS()
class ASYMPTOMAGICKAL_API UAsymGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()
public:
	UAsymGameViewportClient();

	virtual bool WindowCloseRequested() override;

	/**
	* Allow / Disallow the user to close the window without being stopped.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asym Viewport Client")
	static void SetAllowWindowClose(bool bAllowWindowClose);

	/**
	* Can be used to check if the user is currently allowed to close the window without being stopped.
	*/
	UFUNCTION(BlueprintCallable, Category = "Ultimate Window Police")
	static bool GetAllowWindowClose();

	/**
	 * Bind this Event for example in your UI, so when the user requests the close you can open up a
	 * "Do you really want to quit?" window we all love.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ultimate Window Police")
	static void BindOnUserWindowCloseRequested(FOnUserWindowCloseRequested OnUserWindowCloseRequested);

private:
	static bool bAllowWindowClosing;
};
