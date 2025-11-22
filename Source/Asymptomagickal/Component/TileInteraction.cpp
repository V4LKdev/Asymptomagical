// Copyright 2024 Nic, Vlad, Alex


#include "TileInteraction.h"

#include "Asymptomagickal/AsymLogChannels.h"


UTileInteraction::UTileInteraction()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTileInteraction::InitializeComponent()
{
	Super::InitializeComponent();

	
}

bool UTileInteraction::GetTileDataFromHit(const FHitResult& Hit, FTileData& OutTileData)
{
	if (Hit.GetActor())
	{
		if (ITileInterface* TileInterface = Cast<ITileInterface>(Hit.GetActor()))
		{
			OutTileData = TileInterface->GetTileDataFromItem(Hit.Item);
			return true;
		}
	}
	return false;
}


void UTileInteraction::RequestTileTagChange(AActor* GridActor, const int32 TileIndex, const FGameplayTagContainer NewTags)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		UE_LOG(LogAsym, Log, TEXT("Server RequestTileTagChange Called"));
		Internal_RequestTileTagChange(GridActor, TileIndex, NewTags);
	}
	else
	{
		UE_LOG(LogAsym, Log, TEXT("Client RequestTileTagChange Called"));
		Server_RequestTileTagChange(GridActor, TileIndex, NewTags);
	}
}

void UTileInteraction::Server_RequestTileTagChange_Implementation(AActor* GridActor, const int32 TileIndex, const FGameplayTagContainer& NewTags)
{
	UE_LOG(LogAsym, Log, TEXT("RPC Server_RequestTileTagChange Called"));
	Internal_RequestTileTagChange(GridActor, TileIndex, NewTags);
}

void UTileInteraction::Internal_RequestTileTagChange(AActor* GridActor, const int32 TileIndex, const FGameplayTagContainer& NewTags)
{
	UE_LOG(LogAsym, Log, TEXT("Internal_RequestTileTagChange Called"));

	if (ITileInterface* TileInterface = Cast<ITileInterface>(GridActor))
	{
		TileInterface->SetTagsOnTile(TileIndex, NewTags);
	}
}
