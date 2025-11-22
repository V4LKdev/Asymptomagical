// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNicEditorUtilitiesModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	
#pragma region ContentBrowserMenuExtension

	void InitCBMenuExtension();

	TSharedRef<FExtender> CustomCBMenuExtender(const TArray<FString>& SelectedPaths);

	void AddCBMenuEntries(class FMenuBuilder& MenuBuilder);

	void OnCleanupAssetsPressed();
	
	static void FixUpRedirectors();

	void SaveWorldIfDirty();
	
	TArray<FString> FolderPathsSelected;

#pragma endregion //ContentBrowserMenuExtension

#pragma region CommonMaps

	void ExtendContextMenu();
	
	void AddToCommonMapsFromMenu(FName CategoryName);
	
	void CreateCategorySelectionSubmenu(FMenuBuilder& MenuBuilder);
	
	FDelegateHandle ToolMenusHandle;

#pragma endregion //CommonMaps
};
