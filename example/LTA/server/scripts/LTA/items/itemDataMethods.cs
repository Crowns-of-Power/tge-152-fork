// ============================================================
// Project            :  VR20
// File               :  .\LTA\server\scripts\LTA\items\itemDataMethods.cs
// Copyright          :  
// Author             :  Darren
// Created on         :  Thursday, July 19, 2007 2:35 PM
//
// Editor             :  Codeweaver v. 1.2.2685.32755
//
// Description        :  
//                    :  
//                    :  
// ============================================================

datablock ItemData(CashBox)
{
	category = "GameObj";
	shapeFile = "LTA/data/Shapes/Animation/Animation_03_08.dts";
	//lightType = PulsingLight;
	//lightRadius = 1.5;
	//lightColor = "1.0 1.0 0.1 1.0";
		inventoryItem = "Cash";
};

datablock ItemData(MysteryBox01)
{
	category = "GameObj";
	shapeFile = "LTA/data/Shapes/crates/crate01.dts";
	//lightType = PulsingLight;
	//lightRadius = 2;
	//lightColor = "0.1 0.1 1.0 1.0";
		inventoryItem = "Fare";
		respawn = true;
};

datablock ItemData(MysteryBox02)
{
	category = "GameObj";
	shapeFile = "LTA/data/Shapes/crates/crate02.dts";
	//lightType = PulsingLight;
	//lightRadius = 2;
	//lightColor = "0.1 1.0 0.1 1.0";
		inventoryItem = "Bank";
};

function CashBox::onAdd( %DB , %Obj )
{
	Parent::onAdd( %DB , %Obj );
	
	%Obj.static			= true; 
	%Obj.rotate			= true;
}

function CashBox::onPickup( %pickupDB , %pickupObj , %ownerObj )
{
	%name = Parent::onPickup( %pickupDB , %pickupObj , %ownerObj );
	
	if ("" !$= %name)
	{
		%client = %ownerObj.client;
		%value = %ownerObj.myInventory.getInventoryCount(%name);
		%client.cash = %value;
		commandToClient(%client, 'UpdateCash', %client.cash);	
		return true;
	}
	
	return false;
}

function MysteryBox01::onAdd( %DB , %Obj )
{
	Parent::onAdd( %DB , %Obj );
	
	%Obj.static			= true; 
	%Obj.rotate			= true;
}

function MysteryBox01::onPickup( %pickupDB , %pickupObj , %ownerObj )
{
	%name = Parent::onPickup( %pickupDB , %pickupObj , %ownerObj );
	
	if ("" !$= %name)
	{
		%client = %ownerObj.client;
		%value = %ownerObj.myInventory.getInventoryCount(%name);
		%client.fare = %value;
		commandToClient(%client, 'UpdateFare', %client.fare);	
		return true;
	}
	
	return false;
}

function MysteryBox02::onAdd( %DB , %Obj )
{
	Parent::onAdd( %DB , %Obj );
	
	%Obj.static			= true; 
	%Obj.rotate			= true;
}

function MysteryBox02::onPickup( %pickupDB , %pickupObj , %ownerObj )
{
	%name = Parent::onPickup( %pickupDB , %pickupObj , %ownerObj );
	
	if ("" !$= %name)
	{
		%client = %ownerObj.client;
		%value = %ownerObj.myInventory.getInventoryCount(%name);
		%client.bank = %value;
		commandToClient(%client, 'UpdateBank', %client.bank);	
		return true;
	}
	
	return false;
}

/*
//Teoh Declare move marker dts
datablock Itemdata( gMarker )
{
   category = "marker";
   shapeFile = "~/data/shapes/markers/octahedron.dts";
};
*/
