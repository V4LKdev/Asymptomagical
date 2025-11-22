// Fill out your copyright notice in the Description page of Project Settings.


#include "EsotericUser/Public/EsotericUserTypes.h"

#include "OnlineError.h"

void FOnlineResultInformation::FromOnlineError(const FOnlineErrorType& InOnlineError)
{
	bWasSuccessful = InOnlineError.WasSuccessful();
	ErrorId = InOnlineError.GetErrorCode();
	ErrorText = InOnlineError.GetErrorMessage();
}
