// Copyright 2024 Nic, Vlad, Alex


#include "HexInstancedStaticMeshComponent.h"


UHexInstancedStaticMeshComponent::UHexInstancedStaticMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	NumCustomDataFloats = 1;
	
}