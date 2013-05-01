//-----------------------------------------------------------------------------
// RPGDialog - Presents the player with a question and several answers to select from
// Created by Nelson A. K. Gonsalves
//-----------------------------------------------------------------------------

$DialogCheckHandle = 0;

function getAction(%actionFile,%questionNumber)
{
	%file = new FileObject();
	if(isFile($Pref::Server::RPGDialog::ActionPath@%actionFile@".dla") && %file.openForRead($Pref::Server::RPGDialog::ActionPath@%actionFile@".dla"))
	{
		for(%i=1;%i<%questionNumber;%i++)
		{
			%file.readLine();
			if(%file.isEOF())
			{
				%file.close();
				%file.delete();
				return "<Invalid>";
			}
		}
		%action=%file.readLine();
	}
	else
	{
		%file.delete();
		return "<Invalid>";
	}
	
	%file.close();
	%file.delete();
	return %action;
}

function RPGDialogMessageClient(%client,%sender,%npcFile,%questionNumber,%custom)
{
   %senderName=%sender.getshapename();
   %portrait=%sender.RPGDialogPortrait;
   %playerName=%client.player.getshapename();

   %sender.RPGDialogBusy=true;
   %sender.RPGDialogTalkingTo=%client;
   
	if ((%client.player.getObjectMount() != %sender) && (%sender.getObjectMount() != %client.player))
	{
   		%sender.setAimObject(%client.player);
		%client.player.setAimObject(%sender); // AIPlayer only
	}
   
   commandToClient(%client,'RPGDialogMessage',%sender,%senderName,%portrait,%npcFile,%questionNumber,%playerName,%custom);
   if ($DialogCheckHandle == 0) CheckRPGDialogStatus(%client,%sender);
}

function serverCmdRPGDialogAnswer(%client,%sender,%questionNumber,%answerNumber)
{
   if(%client==%sender.RPGDialogTalkingTo)
   {
      %npcFile=%sender.RPGDialogScript;

      %Actions=GetAction(%npcFile,%questionNumber);

      if(%Actions!$="<Invalid>")
      {
		 %answerTag = "<"@%answerNumber@">";
         %ActionHeaderSize=strlen(%answerTag);
         %ActionStart=strPos(%Actions,%answerTag)+%ActionHeaderSize;
         %Actions=getSubStr(%Actions,%ActionStart,strlen(%Actions));
         %ActionEnd=strPos(%Actions,"<"@(%answerNumber+1)@">");

         if(%ActionEnd==-1)
            return;
            
         %Actions=getSubStr(%Actions,0,%ActionEnd);
         
         while(%Actions!$="")
         {
            %ParamStart=strPos(%Actions,"(")+1;
            %ParamEnd=strPos(%Actions,")")-%ParamStart;
            %Param=getSubStr(%Actions,%ParamStart,%ParamEnd);
         
            %Action=getSubStr(%Actions,0,%ParamStart-1);
            if(%Param!$="")
            {
               eval(%Action@"("@%Param@","@%client@","@%sender@",\""@%npcFile@"\");");
            }
            else
            {
               eval(%Action@"("@%client@","@%sender@",\""@%npcFile@"\");");
            }
            %Actions=getSubStr(%Actions,strlen(%Action)+%ParamEnd+2,strlen(%Actions));
         }
      }
      else
      {
         echo("ERROR::Invalid Question/Answer!!\nnpcFile = "@%npcFile@"\nquestionNumber = "@%questionNumber);
      }
   }
}

function serverCmdRPGDialogRay(%client)
{
   %StartPos=%client.player.gettransform();
   %Eye = %client.player.getEyeVector();
   %EndPos = vectorScale(%Eye, -1);
   %EndPos = vectorsub(%StartPos,%EndPos);

   InitContainerRadiusSearch(%EndPos, 2.5, $TypeMasks::PlayerObjectType);

   %rayCast=ContainerSearchNext();
   while(%rayCast != 0 )
   {
      if(%rayCast.RPGDialogScript!$="")
      {
         if(!%rayCast.RPGDialogBusy)
         {
            RPGDialogMessageClient(%client, %rayCast, %rayCast.RPGDialogScript,%rayCast.RPGDialogStartQuestion); //start dialog.
            return;
         }
         else
         {
            if(IsRPGDialogBusy(%rayCast))
            {
               if(%client!=%raycast.RPGDialogTalkingTo)
               {
                  messageClient(%client, '', %rayCast.RPGDialogBusyText, %raycast.RPGDialogTalkingTo.player.getShapeName());
                  return;
               }
               else
                  return;
            }
            else
            {
               RPGDialogMessageClient(%client, %rayCast, %rayCast.RPGDialogScript,%rayCast.RPGDialogStartQuestion); //start dialog.
               return;
            }
         }
      }
      %rayCast=ContainerSearchNext();
   }
}

function CreateRPGDialogBox(%client, %target)
{
	if(%target && %target.RPGDialogScript!$="")
	{
		if(!%target.RPGDialogBusy)
		{
			RPGDialogMessageClient(%client, %target, %target.RPGDialogScript,%target.RPGDialogStartQuestion); //start dialog.
			return;
		}
		else
		{
			if(IsRPGDialogBusy(%target))
			{
				if(%client!=%target.RPGDialogTalkingTo)
				{
					messageClient(%client, '', %target.RPGDialogBusyText, %target.RPGDialogTalkingTo.player.getShapeName());
					return;
				}
				else
					return;
			}
			else
			{
				RPGDialogMessageClient(%client, %target, %target.RPGDialogScript,%target.RPGDialogStartQuestion); //start dialog.
				return;
			}
		}
	}
}

function IsRPGDialogBusy(%AiPlayerID)
{
   InitContainerRadiusSearch(%AiPlayerID.getTransform(), 5, $TypeMasks::PlayerObjectType);
   %rayCastBusyCheck=ContainerSearchNext();
   while(%rayCastBusyCheck != 0 )
   {
      if(%rayCastBusyCheck==(%AiPlayerID.RPGDialogTalkingTo).player)
      {
         return(true);
      }
      %rayCastBusyCheck=ContainerSearchNext();
   }
   return(false);
}

function CheckRPGDialogStatus(%Client,%Sender) //Checks if the player has moved since he started the dialog, moving too far from the sender will cancel the dialog
{
	InitContainerRadiusSearch(%Sender.getTransform(), 5, $TypeMasks::PlayerObjectType);
	%rayCast=ContainerSearchNext();
	while(%rayCast != 0 )
	{
		if(%rayCast==%Client.player)
		{
			$DialogCheckHandle= schedule(1000,0,"CheckRPGDialogStatus",%Client,%Sender);
//echo("Generate new event "@$DialogCheckHandle);   
			return;
		}
		%rayCast=ContainerSearchNext();
	}
	//echo("i'm the culprit!! "@$DialogCheckHandle);
	CommandToClient(%client,'CloseRPGDialog');
	CloseDialog(%Client,%Sender);
}

//------------------------------------------------------------------------------
// RPGDialog Script Functions - functions bellow are made to be used by the
// dialog scripts.
// The commented out lines that start with <ActionList> are used when populating the action
// list on the RPGDialog Editor.
//------------------------------------------------------------------------------

//<ActionList>GotoQuestion(QuestionNumber)
function GotoQuestion(%questionNumber,%client,%sender,%npcFile)
{
   RPGDialogMessageClient(%client, %sender, %npcFile, %questionNumber);
}

//<ActionList>CloseDialog()
function CloseDialog(%client,%sender,%npcFile)
{
	//sendind nothing to the client will close the dialog without anything else happening
	%sender.RPGDialogBusy=false;
	%sender.RPGDialogTalkingTo=0;
	%sender.clearAim();
	%client.player.clearAim(); // AIPlayer only
	if ($DialogCheckHandle != 0)
	{
		//echo("remove event"@$DialogCheckHandle);
		cancel($DialogCheckHandle);
		$DialogCheckHandle = 0;
	}
}

//<ActionList>MoveTo(Position)
function MoveTo(%position,%client,%sender,%npcFile)
{
   %sender.setAimLocation(%position);
   %sender.setMoveDestination(%position);
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>KillPlayer()
function KillPlayer(%client,%sender,%npcFile)
{
   %client.player.kill("Sudden");
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>KillSender()
function KillSender(%client,%sender,%npcFile)
{
   %sender.kill("Sudden");
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>DamagePlayer(Amount)
function DamagePlayer(%DamageAmount,%client,%sender,%npcFile)
{
   %client.player.damage(0, %sender.getposition(), %damageAmount, "Sudden");
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>DamageSender(Amount)
function DamageSender(%DamageAmount,%client,%sender,%npcFile)
{
   %sender.damage(0, %sender.getposition(), %damageAmount, "Sudden");
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>TeleportPlayer(Position)
function TeleportPlayer(%Pos,%client,%sender,%npcFile)
{
   %client.player.setTransform(%Pos);
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>TeleportSender(Position)
function TeleportSender(%Pos,%client,%sender,%npcFile)
{
   %sender.setTransform(%Pos);
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>RenamePlayer(NewName)
function RenamePlayer(%NewName,%client,%sender,%npcFile)
{
   messageAllExcept(%client, -1, 'MsgPlayerRenamed', '\c1%1 is now known as %2.',%client.player.getshapeName(),%NewName);
   messageClient(%client, 'MsgPlayerRenamed', '\c1You are now known as %1.',%NewName);
   %client.player.setshapeName(%NewName);
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>RenameSender(NewName)
function RenameSender(%NewName,%client,%sender,%npcFile)
{
   messageAll('MsgAIRenamed','\c1%1 is now known as %2.',%sender.getshapename(),%NewName);
   %sender.setshapeName(%NewName);
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>ChangeStartQuestion(QuestionNumber)
function ChangeStartQuestion(%NewQuestion,%client,%sender,%npcFile)
{
   %sender.RPGDialogStartQuestion=%NewQuestion;
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>ChangeStartQuestionAndOpen(QuestionNumber)
function ChangeStartQuestionAndOpen(%NewQuestion,%client,%sender,%npcFile)
{
   %sender.RPGDialogStartQuestion=%NewQuestion;
   RPGDialogMessageClient(%client, %sender, %sender.RPGDialogScript,%NewQuestion);
}

//<ActionList>ChangeStartQuestionAndGoto(StartQuestion,GotoQuestion)
function ChangeStartQuestionAndGoto(%NewStartQuestion,%GoTo,%client,%sender,%npcFile)
{
   %sender.RPGDialogStartQuestion=%NewQuestion;
   RPGDialogMessageClient(%client, %sender, %sender.RPGDialogScript,%GoTo);
}

//<ActionList>ChangeScript(ScriptName,QuestionNumber)
function ChangeScript(%NewScript,%StartQuestion,%client,%sender,%npcFile)
{
   if(%NewScript!$="" && isFile($RPGDialogEditorPref::ActionPath@%NewScript@".dla"))
   {
   %sender.RPGDialogScript=%NewScript;
   %sender.RPGDialogStartQuestion=%StartQuestion;
   CloseDialog(%client,%sender,%npcFile);
   }
}

//<ActionList>ChangeScriptAndOpen(ScriptName,QuestionNumber)
function ChangeScriptAndOpen(%NewScript,%StartQuestion,%client,%sender,%npcFile)
{

   if(%NewScript!$="" && isFile($RPGDialogEditorPref::ActionPath@%NewScript@".dla"))
   {
      %sender.RPGDialogScript=%NewScript;
      %sender.RPGDialogStartQuestion=%StartQuestion;
      RPGDialogMessageClient(%client, %sender, %NewScript,%StartQuestion);
   }
}

//<ActionList>ChangePortrait(NewPortrait)
function ChangePortrait(%NewPortrait,%client,%sender,%npcFile)
{
   %sender.RPGDialogPortrait=%NewPortrait;
   CloseDialog(%client,%sender,%npcFile);
}

//<ActionList>ChangePortraitAndGoto(NewPortrait,QuestionNumber)
function ChangePortraitAndGoto(%NewPortrait,%QuestionNumber,%client,%sender,%npcFile)
{
   %sender.RPGDialogPortrait=%NewPortrait;
   RPGDialogMessageClient(%client, %sender, %sender.RPGDialogScript,%QuestionNumber);
}
