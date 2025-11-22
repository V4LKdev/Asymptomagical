// Copyright 2024 Nic, Vlad, Alex

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Iris/ReplicationState/IrisFastArraySerializer.h"
#include "HexInstancedStaticMeshComponent.generated.h"



UCLASS()
class ASYMPTOMAGICKAL_API UHexInstancedStaticMeshComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
public:
	UHexInstancedStaticMeshComponent();
	
};
