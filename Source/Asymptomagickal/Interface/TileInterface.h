// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Iris/ReplicationState/IrisFastArraySerializer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Interface.h"
#include "TileInterface.generated.h"


#pragma region FTileDataArray

/**
 * 	Fast TArray Replication is a custom implementation of NetDeltaSerialize that is suitable for TArrays of UStructs. It offers performance
 *	improvements for large data sets, it serializes removals from anywhere in the array optimally, and allows events to be called on clients
 *	for adds and removals. The downside is that you will need to have game code mark items in the array as dirty, and well as the order of the list
 *	is not guaranteed to be identical between client and server in all cases.
 */
USTRUCT(BlueprintType)
struct FTileData : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tile Data")
	int32 TileIndex = -1;
	UPROPERTY(BlueprintReadOnly, Category = "Tile Data")
	FVector_NetQuantize HexCoordinates; //NetQuantize for 0 floating point precision
	UPROPERTY(BlueprintReadOnly, Category = "Tile Data")
	FGameplayTagContainer GameplayTags = FGameplayTagContainer();

	void PreReplicatedRemove(const struct FTileDataArray& InArraySerializer);
	void PostReplicatedAdd(const struct FTileDataArray& InArraySerializer);
	void PostReplicatedChange(const struct FTileDataArray& InArraySerializer);

	bool operator==(const int32 OtherIndex) const
	{
		return TileIndex == OtherIndex;
	}
};


USTRUCT(BlueprintType)
struct FTileDataArray : public FIrisFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tile Data Array")
	TArray<FTileData> Items;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParams)
	{
		return FastArrayDeltaSerialize<FTileData, FTileDataArray>( Items, DeltaParams, *this );
	}
};

template<>
struct TStructOpsTypeTraits<FTileDataArray> : public TStructOpsTypeTraitsBase2<FTileDataArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

#pragma endregion // FTileDataArray


UINTERFACE(NotBlueprintable)
class UTileInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ASYMPTOMAGICKAL_API ITileInterface
{
	GENERATED_BODY()

public:
	virtual FTileData GetTileDataFromItem(const int32 Item) = 0;
	
	virtual void SetTagsOnTile(const int32 TileIndex, const FGameplayTagContainer& Tags) = 0;
};
