// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EsotericUserSubsystem.h"
#include "Engine/CancellableAsyncAction.h"
#include "AsyncAction_EsotericUserInit.generated.h"

enum class EEsotericUserOnlineContext : uint8;
enum class EEsotericUserPrivilege : uint8;
struct FInputDeviceId;

/**
 * Async action to handle different functions for initializing users
 */
UCLASS()
class ESOTERICUSER_API UAsyncAction_EsotericUserInit : public UCancellableAsyncAction
{
	GENERATED_BODY()
public:
	/**
	 * Initializes a local player with the Esoteric user system, which includes doing platform-specific login and privilege checks.
	 * When the process has succeeded or failed, it will broadcast the OnInitializationComplete delegate.
	 *
	 * @param LocalPlayerIndex	Desired index of ULocalPlayer in Game Instance, 0 will be primary player and 1+ for local multiplayer
	 * @param PrimaryInputDevice Primary input device for the user, if invalid will use the system default
	 * @param bCanUseGuestLogin	If true, this player can be a guest without a real system net id
	 */
	UFUNCTION(BlueprintCallable, Category = EsotericUser, meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_EsotericUserInit* InitializeForLocalPlay(UEsotericUserSubsystem* Target, int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin);

	/**
	 * Attempts to log an existing user into the platform-specific online backend to enable full online play
	 * When the process has succeeded or failed, it will broadcast the OnInitializationComplete delegate.
	 *
	 * @param LocalPlayerIndex	Index of existing LocalPlayer in Game Instance
	 */
	UFUNCTION(BlueprintCallable, Category = EsotericUser, meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_EsotericUserInit* LoginForOnlinePlay(UEsotericUserSubsystem* Target, int32 LocalPlayerIndex);

	/** Call when initialization succeeds or fails */
	UPROPERTY(BlueprintAssignable)
	FEsotericUserOnInitializeCompleteMulticast OnInitializationComplete;

	/** Fail and send callbacks if needed */
	void HandleFailure();

	/** Wrapper delegate, will pass on to OnInitializationComplete if appropriate */
	UFUNCTION()
	virtual void HandleInitializationComplete(const UEsotericUserInfo* UserInfo, bool bSuccess, FText Error, EEsotericUserPrivilege RequestedPrivilege, EEsotericUserOnlineContext OnlineContext);

protected:
	/** Actually start the initialization */
	virtual void Activate() override;

	TWeakObjectPtr<UEsotericUserSubsystem> Subsystem;
	FEsotericUserInitializeParams Params;
};
