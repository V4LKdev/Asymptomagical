// Copyright 2024 Nic Vlad, Alex

#pragma once
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNicEditorUtility, Log, All);
DEFINE_LOG_CATEGORY(LogNicEditorUtility);

namespace UtilityDebug
{

	inline void Print(const FString& Message, const FColor& Color)
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, Message);
		}
	}
	
	inline void PrintLog(const FString& Message)
    {
    	UE_LOG(LogNicEditorUtility, Warning, TEXT("%s"), *Message);
    }
    
    inline EAppReturnType::Type ShowMessageDialog(const EAppMsgType::Type MsgType, const FString& Message, const bool bShowMsgAsWarning = true)
    {
    	if(bShowMsgAsWarning)
    	{
    		const FText MsgTitle = FText::FromString("Final Warning");
    
    		return FMessageDialog::Open(MsgType, FText::FromString(Message), MsgTitle);
    	}
    	else
    	{
    		return FMessageDialog::Open(MsgType, FText::FromString(Message));
    	}
    }
    
    inline void ShowNotifyInfo(const FString& Message)
    {
    	FNotificationInfo NotificationInfo(FText::FromString(Message));
    	NotificationInfo.bUseLargeFont = true;
    	NotificationInfo.FadeOutDuration = 7.f;
    	
    	FSlateNotificationManager::Get().AddNotification(NotificationInfo);
    }
}