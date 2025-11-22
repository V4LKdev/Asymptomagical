// Copyright 2024 Nic Vlad, Alex

#pragma once
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

namespace AsymUtilities
{
	static void LoadSoftClassReferenceAsync(TSoftClassPtr<UObject> SoftClassRef, TFunction<void(UClass*)> OnLoaded)
	{
		if (SoftClassRef.IsValid())
		{
			// If the class is already loaded, invoke the callback immediately
			OnLoaded(SoftClassRef.Get());
			return;
		}

		// Use the StreamableManager to request async loading
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
        
		// Request async load of the soft reference
		Streamable.RequestAsyncLoad(SoftClassRef.ToSoftObjectPath(),
			FStreamableDelegate::CreateLambda([SoftClassRef, OnLoaded]()
			{
				if (UClass* LoadedClass = SoftClassRef.Get())
				{
					// Successfully loaded, invoke the callback
					OnLoaded(LoadedClass);
				}
				else
				{
					// Handle failure (you can extend this to pass an error to the callback)
					UE_LOG(LogTemp, Warning, TEXT("Failed to load the class!"));
					OnLoaded(nullptr);
				}
			}));
	}
};

