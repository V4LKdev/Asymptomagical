// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CommonMapsDeveloperSettings.generated.h"

USTRUCT(BlueprintType)
struct FCommonMapContainer
{
	GENERATED_BODY()
public:
	FCommonMapContainer() = default;
	explicit FCommonMapContainer(const TArray<FSoftObjectPath>& InPaths) : MapURL(InPaths){}
public:
	UPROPERTY(EditAnywhere, Category="", meta=(AllowedClasses="/Script/Engine.World"))
	TSet<FSoftObjectPath> MapURL;
};

/**
 * 
 */
UCLASS(config=EditorPerProjectUserSettings, DisplayName="Common Maps")
class NICEDITORUTILITIES_API UCommonMapsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(Config,EditAnywhere, Category="Common Maps|Maps", meta=(ForceInlineRow))
	TMap<FName, FCommonMapContainer> Maps;
#endif
};
