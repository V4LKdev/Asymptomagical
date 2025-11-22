// Copyright 2024 Nic, Vlad, Alex


#include "HexGrid.h"

#include "Asymptomagickal/AsymGameplayTags.h"
#include "Asymptomagickal/AsymLogChannels.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

static float GSqrt3 = FMath::Sqrt(3.f);

#pragma region Editor

void AHexGrid::CreateHexGrid() const
{
	ISMC->ClearInstances();
	if (HexMesh)
	{
		ISMC->SetStaticMesh(HexMesh);
	}
	
	const float Height = Radius * GSqrt3;

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		float XOffset = Height * Row;

		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const float YOffset = Radius * 1.5f * Column;

			FVector Location = Column % 2 == 0 ?
				FVector(XOffset, YOffset, 0.f) :
				FVector(Height * 0.5f + XOffset, YOffset, 0.f);
			
			int32 InstanceIndex = ISMC->AddInstance(FTransform(Location));

			ISMC->SetCustomDataValue(InstanceIndex, 0, 0); //Custom data will probably be replaced by the gameplaytag container
		}
	}
}

void AHexGrid::RaiseRim() const
{
	const float Height = Radius * GSqrt3;
	
	// Raise all hexes on the rim
	for (int32 InstanceIndex = 0; InstanceIndex < ISMC->GetInstanceCount(); ++InstanceIndex)
	{
		FTransform InstanceTransform;
		ISMC->GetInstanceTransform(InstanceIndex, InstanceTransform);

		const FVector Location = InstanceTransform.GetLocation();
		const float X = Location.X;
		const float Y = Location.Y;

		if (X == 0 || X == Height * (Rows - 1) || Y == 0 || Y == Radius * 1.5f * (Columns - 1) || X == Height * 0.5f || X == Height * 0.5f + Height * (Rows - 1))
		{
			// Randomize height
			const float RandomHeight = FMath::RandRange(2000, 2400);
			InstanceTransform.SetLocation(FVector(Location.X, Location.Y, RandomHeight));
			ISMC->UpdateInstanceTransform(InstanceIndex, InstanceTransform);
			ISMC->SetCustomDataValue(InstanceIndex, 0, RandomHeight);
		}
	}
}

void AHexGrid::RandomizeHeight() const
{
	for (int32 InstanceIndex = 0; InstanceIndex < ISMC->GetInstanceCount(); ++InstanceIndex)
	{
		FTransform InstanceTransform;
		ISMC->GetInstanceTransform(InstanceIndex, InstanceTransform);

		const FVector Location = InstanceTransform.GetLocation();
		const float RandomHeight = FMath::RandRange(-RandomSpan, RandomSpan);

		InstanceTransform.SetLocation(FVector(Location.X, Location.Y, RandomHeight));
		ISMC->UpdateInstanceTransform(InstanceIndex, InstanceTransform);
		ISMC->SetCustomDataValue(InstanceIndex, 0, RandomHeight);
	}
	
}

void AHexGrid::Clear() const
{
	ISMC->ClearInstances();
}

#pragma endregion // Editor


AHexGrid::AHexGrid()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	TileArray.OwningObject = this;

	ISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridInstancedMesh"));
	RootComponent = ISMC;
	if(HexMesh)
	{
		ISMC->SetStaticMesh(HexMesh);
		ISMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AHexGrid::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// TODO: Benchmark this
	FDoRepLifetimeParams AHexGridParams;
	AHexGridParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(AHexGrid, TileArray, AHexGridParams);
	// Or
	//DOREPLIFETIME(AHexGrid, TileArray);
}

void AHexGrid::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogAsym, Log, TEXT("BeginPlay HexGrid"));

	InitInstancesLocally();

	InitializeHexGrid();
}

void AHexGrid::InitInstancesLocally()
{
	if(HasAuthority())
	{
		TileArray.Items.Empty();
	}
	
	ISMC->ClearInstances();
	
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const int32 InstanceIndex = ISMC->AddInstance(FTransform());

			if (HasAuthority())
			{
				FTileData NewTile;
				NewTile.TileIndex = InstanceIndex;
				NewTile.HexCoordinates = FVector(0.f, 0.f, 0.f);
				NewTile.GameplayTags.AddTag(AsymGameplayTags::Tile_Permission_All);
				TileArray.Items.Add(NewTile);
			}
		}
	}

	UE_LOG(LogAsym, Log, TEXT("Initialized Instances Locally"));
}

void AHexGrid::InitializeHexGrid()
{
	if (!HasAuthority())
	{
		return;
	}

	// Go over the tile array and set the positions of the instances to a grid and then dirty the array

	for (int32 i = 0; i < TileArray.Items.Num(); ++i)
	{
		const FTileData& Tile = TileArray.Items[i];
		const int32 InstanceIndex = Tile.TileIndex;

		const int32 Row = i / Columns;
		const int32 Column = i % Columns;

		const float Height = Radius * GSqrt3;
		const float XOffset = Height * Row;
		const float YOffset = Radius * 1.5f * Column;

		FVector_NetQuantize Location = Column % 2 == 0 ?
			FVector(XOffset, YOffset, 0.f) :
			FVector(Height * 0.5f + XOffset, YOffset, 0.f);

		FTransform InstanceTransform;
		InstanceTransform.SetLocation(Location);
		ISMC->UpdateInstanceTransform(InstanceIndex, InstanceTransform);

		TileArray.Items[i].HexCoordinates = Location;
	}

	TileArray.MarkArrayDirty();

	UE_LOG(LogAsym, Log, TEXT("Initialized HexGrid on Server"));
}


FTileData AHexGrid::GetTileDataFromItem(const int32 Item)
{
	return GetTileFromIndex(Item);
}

void AHexGrid::SetTagsOnTile(const int32 TileIndex, const FGameplayTagContainer& NewTags)
{
	if(!HasAuthority())
	{
		UE_LOG(LogAsym, Error, TEXT("SetTagsOnTile cannot be called on client"));
		return;
	}
	
	for (FTileData& Tile : TileArray.Items)
	{
		if (Tile == TileIndex)
		{
			Tile.GameplayTags = NewTags;
			TileArray.MarkItemDirty(Tile);
			return;
		}
	}

	UE_LOG(LogAsym, Warning, TEXT("Tile with index %d not found"), TileIndex);
}



FTileData AHexGrid::GetTileFromIndex(const int32 Index) const
{
	for (const FTileData& Tile : TileArray.Items)
	{
		if (Tile == Index)
		{
			return Tile;
		}
	}

	UE_LOG(LogAsym, Warning, TEXT("Tile with index %d not found"), Index);
	return FTileData();
}



#pragma region FTileData

// TODO: Try to find a way to Bulk call this function if the bandwith gets too intense with many modifications simultaniously
void FTileData::PostReplicatedAdd(const FTileDataArray& InArraySerializer)
{
	AHexGrid* HexGrid = Cast<AHexGrid>(InArraySerializer.OwningObject);
	if (HexGrid && HexGrid->ISMC)
	{
		FTransform InstanceTransform;
		InstanceTransform.SetLocation(HexCoordinates);
		HexGrid->ISMC->UpdateInstanceTransform(TileIndex, InstanceTransform);

		UE_LOG(LogAsym, Log, TEXT("PostReplicatedAdd %s"), *HexCoordinates.ToString());
	}
}

void FTileData::PreReplicatedRemove(const struct FTileDataArray& InArraySerializer)
{
}

void FTileData::PostReplicatedChange(const struct FTileDataArray& InArraySerializer)
{
}

#pragma endregion // FTileData
