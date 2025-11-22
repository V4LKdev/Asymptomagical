// Copyright 2024 Nic, Vlad, Alex


#include "AsymActivatableWidgetStack.h"
#include "Asymptomagickal/AsymLogChannels.h"

#if 0

void UAsymActivatableWidgetStack::PushWidgetToStack(TSubclassOf<UAsymActivatableWidget> WidgetClass)
{

	UAsymActivatableWidget* Widget = AddWidget<UAsymActivatableWidget>(WidgetClass);

	UAsymUserWidget* ParentWidget = GetTypedOuter<UAsymUserWidget>();
	
	if (ensure(ParentWidget) && ParentWidget->Implements<UAsymUIInterface>())
	{
		TScriptInterface<IAsymUIInterface> StackOwnerInterface;
		StackOwnerInterface.SetObject(ParentWidget);
		StackOwnerInterface.SetInterface(Cast<IAsymUIInterface>(ParentWidget));
		
		ensure(StackOwnerInterface.GetInterface());
		Widget->SetOwnerInterface(StackOwnerInterface);
		
		return;
	}
	UE_LOG(LogAsym, Error, TEXT("Couldn't find parent with AsymUIInterface to assign!"));

}
#endif
