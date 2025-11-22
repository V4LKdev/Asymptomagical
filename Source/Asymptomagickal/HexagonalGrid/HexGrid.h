// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "Asymptomagickal/Interface/TileInterface.h"
#include "GameFramework/Actor.h"
#include "HexGrid.generated.h"

/**
 * Server Authoritative Actor Class that has an IMC to create a Hexagonal Grid
 */
UCLASS()
class ASYMPTOMAGICKAL_API AHexGrid : public AActor, public ITileInterface
{
	GENERATED_BODY()

public:
	AHexGrid();

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UInstancedStaticMeshComponent> ISMC;
	
	/** ITileInterface **/
	UFUNCTION()
	virtual FTileData GetTileDataFromItem(const int32 Item) override;
	UFUNCTION()
	virtual void SetTagsOnTile(const int32 TileIndex, const FGameplayTagContainer& NewTags) override;

	
protected:
	virtual void BeginPlay() override;

	//Editor functions for grid manipulation
	UFUNCTION(CallInEditor, Category="HexGrid")
	void CreateHexGrid() const;
	UFUNCTION(CallInEditor, Category="HexGrid")
	void RaiseRim() const;
	UFUNCTION(CallInEditor, Category="HexGrid")
	void RandomizeHeight() const;
	UFUNCTION(CallInEditor, Category="HexGrid")
	void Clear() const;

protected:
	UPROPERTY(EditAnywhere, Category="HexGrid")
	TObjectPtr<UStaticMesh> HexMesh;

	//Fast TArray for replication
	UPROPERTY(Replicated)
	FTileDataArray TileArray;

private:
	void InitInstancesLocally();
	
	void InitializeHexGrid();
	
	FTileData GetTileFromIndex(int32 Index) const;

private:
	UPROPERTY(EditAnywhere, Category="HexGrid", meta=(AllowPrivateAccess="true"))
	float Radius = 50.f;
	UPROPERTY(EditAnywhere, Category="HexGrid", meta=(AllowPrivateAccess="true"))
	int32 Rows = 4;
	UPROPERTY(EditAnywhere, Category="HexGrid", meta=(AllowPrivateAccess="true"))
	int32 Columns = 4;
	UPROPERTY(EditAnywhere, Category="HexGrid", meta=(AllowPrivateAccess="true"))
	float RandomSpan = 10.f;
};
