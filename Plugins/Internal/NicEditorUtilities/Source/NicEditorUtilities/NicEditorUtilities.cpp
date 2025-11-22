// Copyright Epic Games, Inc. All Rights Reserved.

#include "NicEditorUtilities.h"

#include "AssetSelection.h"
#include "AssetToolsModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserModule.h"
#include "DebugHeader.h"
#include "ObjectTools.h"
#include "EditorAssetLibrary.h"
#include "FileHelpers.h"
#include "LevelEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CommonMaps/CommonMapsDeveloperSettings.h"

#define LOCTEXT_NAMESPACE "FNicEditorUtilitiesModule"

#pragma region CommonMaps

namespace CommonMapsFunctionLibrary
{
	static bool HasPlayWorld()
	{
		return GEditor->PlayWorld != nullptr;
	}

	static bool HasNoPlayWorld()
	{
		return !HasPlayWorld();
	}

	static void OpenCommonMap_Clicked(const FString MapPath)
	{
		constexpr bool bPromptUserToSave = true;
		constexpr bool bSaveMapPackages = true;
		constexpr bool bSaveContentPackages = true;
		constexpr bool bFastSave = false;
		constexpr bool bNotifyNoPackagesSaved = false;
		constexpr bool bCanBeDeclined = true;
		if (FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave,
		                                        bNotifyNoPackagesSaved, bCanBeDeclined))
		{
			if (ensure(MapPath.Len()))
			{
				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(MapPath);
			}
		}
	}

	static TSharedRef<SWidget> GetCommonMapsDropdown()
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		for (const auto& [CategoryName, Maps] : GetDefault<UCommonMapsDeveloperSettings>()->Maps)
		{
			TAttribute<FText> SectionText;
			SectionText.Set(FText::FromName(CategoryName));
			MenuBuilder.BeginSection(NAME_None, SectionText);
			for (const auto& Map : Maps.MapURL)
			{
				if (!Map.IsValid())
				{
					continue;
				}

				const FText DisplayName = FText::FromString(Map.GetAssetName());
				MenuBuilder.AddMenuEntry(
					DisplayName,
					FText::Format(LOCTEXT("CommonPathDescription", "{0}"), FText::FromString(Map.ToString())),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateStatic(&CommonMapsFunctionLibrary::OpenCommonMap_Clicked, Map.ToString()),
						FCanExecuteAction::CreateStatic(&CommonMapsFunctionLibrary::HasNoPlayWorld),
						FIsActionChecked(),
						FIsActionButtonVisible::CreateStatic(&CommonMapsFunctionLibrary::HasNoPlayWorld)
					)
				);
			}
			MenuBuilder.EndSection();
		}

		return MenuBuilder.MakeWidget();
	}

	static bool CanShowCommonMaps()
	{
		return HasNoPlayWorld() && !GetDefault<UCommonMapsDeveloperSettings>()->Maps.IsEmpty();
	}

	static void RegisterGameEditorMenus()
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		FToolMenuSection& Section = Menu->AddSection("PlayGameExtensions", TAttribute<FText>(),
		                                             FToolMenuInsert("Play", EToolMenuInsertType::Default));


		FToolMenuEntry CommonMapEntry = FToolMenuEntry::InitComboButton(
			"CommonMapOptions",
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateStatic(&CommonMapsFunctionLibrary::HasNoPlayWorld),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateStatic(&CommonMapsFunctionLibrary::CanShowCommonMaps)),
			FOnGetContent::CreateStatic(&CommonMapsFunctionLibrary::GetCommonMapsDropdown),
			LOCTEXT("CommonMaps_Label", "Common Maps"),
			LOCTEXT("CommonMaps_ToolTip", "Some commonly desired maps while using the editor"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Level")
		);
		CommonMapEntry.StyleNameOverride = "CalloutToolbar";
		Section.AddEntry(CommonMapEntry);
	}
}

#pragma endregion //CommonMaps

void FNicEditorUtilitiesModule::StartupModule()
{
	InitCBMenuExtension();

	ExtendContextMenu();

	if (!IsRunningGame())
	{
		if (FSlateApplication::IsInitialized())
		{
			ToolMenusHandle = UToolMenus::RegisterStartupCallback(
				FSimpleMulticastDelegate::FDelegate::CreateStatic(&CommonMapsFunctionLibrary::RegisterGameEditorMenus));
		}
	}
}

void FNicEditorUtilitiesModule::ShutdownModule()
{
}

#pragma region ContentBrowserMenuExtension

void FNicEditorUtilitiesModule::InitCBMenuExtension()
{
	FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	TArray<FContentBrowserMenuExtender_SelectedPaths>& CBMenuExtenders = CBModule.GetAllPathViewContextMenuExtenders();
	
	CBMenuExtenders.Add(
		FContentBrowserMenuExtender_SelectedPaths::CreateRaw(
		this, &FNicEditorUtilitiesModule::CustomCBMenuExtender)
		);
}

TSharedRef<FExtender> FNicEditorUtilitiesModule::CustomCBMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> MenuExtender (new FExtender());

	if(SelectedPaths.Num()>0)
	{
		MenuExtender->AddMenuExtension(FName("Delete"),
		EExtensionHook::After,
		TSharedPtr<FUICommandList>(),
		FMenuExtensionDelegate::CreateRaw(this, &FNicEditorUtilitiesModule::AddCBMenuEntries));

		FolderPathsSelected = SelectedPaths;
	}
	
	return MenuExtender;
}

void FNicEditorUtilitiesModule::AddCBMenuEntries(class FMenuBuilder& MenuBuilder)
{
	const FText CleanupAssetsLabel = FText::FromString("Cleanup Assets");
	const FText CleanupAssetsTooltip = FText::FromString("Safely deletes all unused assets and empty folders under this directory");
	
	MenuBuilder.AddMenuEntry
	(
		CleanupAssetsLabel,
		CleanupAssetsTooltip,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.PrivateContentEdit"),
		FExecuteAction::CreateRaw(this, &FNicEditorUtilitiesModule::OnCleanupAssetsPressed)
	);
}

void FNicEditorUtilitiesModule::OnCleanupAssetsPressed()
{
	TSet<FString> AssetPaths;
	
	for(FString& FolderPath : FolderPathsSelected)
	{
		TArray<FString> FolderPaths = UEditorAssetLibrary::ListAssets(FolderPath, true, true);

		for (const FString& Path : FolderPaths)
		{
			AssetPaths.Add(Path);
		}
	}
	
	if (AssetPaths.Num() > 50)
	{
		const EAppReturnType::Type ConfirmedResult = UtilityDebug::ShowMessageDialog(
		EAppMsgType::OkCancel,
		FString::Printf(TEXT("The operation will execute on over %i elements. \n The editor might become unresponsive. \n not because im bad at optimizing, trust"),
			AssetPaths.Num()));

		if(ConfirmedResult == EAppReturnType::Cancel)
		{
			return;
		}
	}

	if(AssetPaths.Num()<=0)
	{
		//* Todo: Delete Directory */
		UtilityDebug::PrintLog(TEXT("No assets found in selected directory"));
		return;
	}

	FixUpRedirectors();
	SaveWorldIfDirty();

	
	for (FString& AssetPath : AssetPaths)
	{
		UEditorAssetLibrary::SaveAsset(AssetPath, false);
	}

	/// Delete Unused Assets
	TArray<FAssetData> UnusedAssetDataArray;

	for (const FString& AssetPath : AssetPaths)
	{
		if (AssetPath.Contains(TEXT("Collections")) ||
			AssetPath.Contains(TEXT("Developers")) ||
			AssetPath.Contains(TEXT("__ExternalActors__")) ||
			AssetPath.Contains(TEXT("__ExternalObjects__")))
		{
			continue;
		}

		if(!UEditorAssetLibrary::DoesAssetExist(AssetPath) || UEditorAssetLibrary::LoadAsset(AssetPath)->IsA<UWorld>())
		{
			continue;
		}

		TArray<FString> Referencers = UEditorAssetLibrary::FindPackageReferencersForAsset(AssetPath);

		if(Referencers.Num() <= 0)
		{
			const FAssetData UnusedAssetData = UEditorAssetLibrary::FindAssetData(AssetPath);
			UnusedAssetDataArray.Add(UnusedAssetData);
		}
	}

	if(UnusedAssetDataArray.Num()>0)
	{
		ObjectTools::DeleteAssets(UnusedAssetDataArray);
	}
	else
	{
		UtilityDebug::ShowMessageDialog(EAppMsgType::Ok, TEXT("No unreferenced assets found"));
	}


	/// Delete empty Folders
	FixUpRedirectors();
	
	TArray<FString> EmptyFoldersPathsArray;

	for (const FString& FolderPath : AssetPaths)
	{
		if (FolderPath.Contains(TEXT("Collections")) ||
			FolderPath.Contains(TEXT("Developers")) ||
			FolderPath.Contains(TEXT("__ExternalActors__")) ||
			FolderPath.Contains(TEXT("__ExternalObjects__")))
		{
			continue;
		}

		if(!UEditorAssetLibrary::DoesDirectoryExist(FolderPath)) continue;

		if(!UEditorAssetLibrary::DoesDirectoryHaveAssets(FolderPath))
		{
			EmptyFoldersPathsArray.Add(FolderPath);
		}	
	}

	for(const FString& EmptyFolderPath:EmptyFoldersPathsArray)
	{
		UEditorAssetLibrary::DeleteDirectory(EmptyFolderPath);
	}
}


void FNicEditorUtilitiesModule::FixUpRedirectors()
{
	// Load module
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
     
	// Create filter with asset paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassPaths.Add(UObjectRedirector::StaticClass()->GetClassPathName());
     
	// Query for assets in selected paths
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);
     
	if (AssetList.Num() == 0) return;
     
	// Get paths for each asset
	TArray<FString> ObjectPaths;
     
	for (const FAssetData& Asset : AssetList)
	{
		ObjectPaths.Add(Asset.GetObjectPathString());
	}
     
	// Load assets
	TArray<UObject*> Objects;

	AssetViewUtils::FLoadAssetsSettings Settings;
	Settings.bFollowRedirectors = false;
	Settings.bAllowCancel = true;

	if (const AssetViewUtils::ELoadAssetsResult Result = LoadAssetsIfNeeded(ObjectPaths, Objects, Settings); Result != AssetViewUtils::ELoadAssetsResult::Cancelled)
	{
		// Convert objects to object redirectors
		TArray<UObjectRedirector*> Redirectors;
		for (UObject* Object : Objects)
		{
			Redirectors.Add(CastChecked<UObjectRedirector>(Object));
		}
     
		// Call fix up redirectors from asset tools
		const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
     
		AssetToolsModule.Get().FixupReferencers(Redirectors);
	}
}

void FNicEditorUtilitiesModule::SaveWorldIfDirty()
{
	if (!GEditor)
	{
		return;
	}

	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		// LevelEditorSubsystem->SaveCurrentLevel();
		if (const UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			if (const ULevel* CurrentLevel = World->GetCurrentLevel())
			{
				if (const UPackage* DirtyMapPackage = CurrentLevel->GetOutermost(); DirtyMapPackage->IsDirty())
				{
					LevelEditorSubsystem->SaveCurrentLevel();
				}
			}
		}
	}
}

#pragma endregion //ContentBrowserMenuExtension

#pragma region CommonMaps

void FNicEditorUtilitiesModule::ExtendContextMenu()
{
	UToolMenu* WorldAssetMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.World");
	if (!WorldAssetMenu) return;

	FToolMenuSection& Section = WorldAssetMenu->AddSection("AssetContextCommonMaps", LOCTEXT("CommonMapsOptionsMenuHeading", "Esoteric Instinct"));
	
	Section.AddSubMenu(FName("Common Maps"),
		LOCTEXT("AddToCommonMapsFromMenu", "Add to Common Maps"),
		LOCTEXT("AddToCommonMapsFromMenuTooltip", "Add this map into Common Maps list."),
		FNewMenuDelegate::CreateRaw(this, &FNicEditorUtilitiesModule::CreateCategorySelectionSubmenu),
		FToolUIActionChoice(),
		EUserInterfaceActionType::None,
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Star"));
}

void FNicEditorUtilitiesModule::AddToCommonMapsFromMenu(FName CategoryName)
{
	TArray<FAssetData> SelectedAssets;
	AssetSelectionUtils::GetSelectedAssets(SelectedAssets);
	if (auto* Settings = GetMutableDefault<UCommonMapsDeveloperSettings>())
	{
		for (const auto& AssetData : SelectedAssets)
		{
			if (FCommonMapContainer* CurrentMaps = Settings->Maps.Find(CategoryName))
				CurrentMaps->MapURL.Add(AssetData.GetSoftObjectPath());
			
			else
				Settings->Maps.Add(CategoryName, FCommonMapContainer({AssetData.GetSoftObjectPath()}));
		}
		
		Settings->SaveConfig(CPF_Config);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("All maps added to Common Maps!"));
	}
}

void FNicEditorUtilitiesModule::CreateCategorySelectionSubmenu(FMenuBuilder& MenuBuilder)
{
	const UCommonMapsDeveloperSettings* DevSettings = GetDefault<UCommonMapsDeveloperSettings>();
	if (!DevSettings) return;

	TArray<FName> Categories;
	DevSettings->Maps.GetKeys(Categories);
	for (const FName& CategoryName : Categories)
	{
		MenuBuilder.AddMenuEntry(
			FText::Format(LOCTEXT("CategoryLabel", "{0}"), FText::FromName(CategoryName)),
			FText::Format(LOCTEXT("CategoryTooltip", "Add this map to \"{0}\" category."), FText::FromName(CategoryName)),
			FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FNicEditorUtilitiesModule::AddToCommonMapsFromMenu, CategoryName)));
	}
}

#pragma endregion //CommonMaps

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNicEditorUtilitiesModule, NicEditorUtilities)