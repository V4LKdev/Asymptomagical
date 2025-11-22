// Copyright 2024 Nic Vlad, Alex

#pragma once

#include "Logging/LogMacros.h"

class UObject;

ASYMPTOMAGICKAL_API DECLARE_LOG_CATEGORY_EXTERN(LogAsym, Log, All);
ASYMPTOMAGICKAL_API DECLARE_LOG_CATEGORY_EXTERN(LogAsymAbilitySystem,Log, All);

ASYMPTOMAGICKAL_API FString GetClientServerContextString(UObject* ContextObject = nullptr);