// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Asymptomagickal/Interface/TileInterface.h"
#include "Components/ActorComponent.h"
#include "TileInteraction.generated.h"

/*
* The Interaction Component that allows Clients to send change requests to the server autoritative version of the grid
*/
UCLASS(ClassGroup=(Asmyptomagickal), meta=(BlueprintSpawnableComponent))
class ASYMPTOMAGICKAL_API UTileInteraction : public UActorComponent
{
	GENERATED_BODY()
public:
	UTileInteraction();

	virtual void InitializeComponent() override;

	UFUNCTION(BlueprintCallable, Category="Tile Interaction")
	bool GetTileDataFromHit(const FHitResult& Hit, FTileData& OutTileData);

	UFUNCTION(BlueprintCallable, Category="Tile Interaction")
	void RequestTileTagChange(AActor* GridActor, const int32 TileIndex, const FGameplayTagContainer NewTags);
	
private:
	UFUNCTION(Server, Reliable)
	void Server_RequestTileTagChange(AActor* GridActor, const int32 TileIndex, const FGameplayTagContainer& NewTags);

	void Internal_RequestTileTagChange(AActor* GridActor, const int32 TileIndex, const FGameplayTagContainer& NewTags);
};
