//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// MainMenuGui is the main Control through which the game shell is viewed.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// not being call at all?
function MainMenuGui::onAdd(%this)
{
	echo(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MainMenuGui::onAdd");
}

// call when program end
function MainMenuGui::onRemove(%this)
{
}

// when add in when show
function MainMenuGui::onWake(%this)
{
	Canvas.setCursor("SwordCursor");
}

// when hide
function MainMenuGui::onSleep(%this)
{
	Canvas.setCursor("DefaultCursor");
}
