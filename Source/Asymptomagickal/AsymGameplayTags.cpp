// Copyright 2024 Nic, Vlad, Alex


#include "AsymGameplayTags.h"

namespace AsymGameplayTags
{
	/*
	 * Input Tags 
	 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look input for Mouse.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Stick, "InputTag.Look.Stick", "Look input for Stick.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump Input");

	/*
	*	UI Layers
	*/
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu, "UI.Layer.Menu", "Menu UI Layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_PopUp, "UI.Layer.PopUp", "PopUp UI Layer.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_HUD, "UI.Layer.HUD", "HUD UI Layer.");
	
	/*
	 *	Tile Tags
	 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tile_Permission_Locked, "Tile.Permission.Locked", "Tile is locked.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tile_Permission_OnlyKing, "Tile.Permission.OnlyKing", "Only the King can access this tile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tile_Permission_OnlyPlayers, "Tile.Permission.OnlyPlayers", "Only Players can access this tile.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tile_Permission_All, "Tile.Permission.All", "All can access this tile.");
	
}
