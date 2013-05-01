//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

function initEditor()
{
   exec("~/ui/MainEditorScreenGui.gui");
   exec("~/ui/EditQuestionGui.gui");
   exec("~/ui/EditAnswerGui.gui");
   exec("~/ui/NewScriptPopup.gui");
   exec("~/ui/SetPathsPopup.gui");
   exec("~/ui/EditorOpeningGui.gui");

   exec("~/defaults.cs");
   exec("~/prefs.cs");
   exec("~/editorMain.cs");
}

function startEditor()
{
   // The client mod has already set it's own content, but we'll
   // just load something new.
   Canvas.setContent(EditorOpeningGui);
   Canvas.setCursor("DefaultCursor");
   PopulateActionList();
   PopulateQuestionOptionsList();
}


//-----------------------------------------------------------------------------
// Package overrides to initialize the mod.
// This module currently loads on top of the client mod, but it probably
// doesn't need to.  Should look into having disabling the client and
// doing our own canvas init.

package RPGDialogEditor {

function onStart()
{
   Parent::onStart();
   echo("\n--------- Initializing MOD: RPGDialogEditor ---------");

   if (!isObject(Canvas))  {
      // If the parent onStart didn't open a canvas, then we're
      // probably not running as a mod.  We'll have to do the work
      // ourselves.
      initCanvas("RPGDialog Editor");
   }
   initEditor();
   startEditor();
}

function onExit()
{
   echo("Exporting RPGDialog editor prefs");
   export("$RPGDialogEditorPref::*", "~/prefs.cs", False);

   if(isEventPending($RPGDialog::RefreshSchedule))
      cancel($RPGDialog::RefreshSchedule);
      
   Parent::onExit();
}

}; // package end.
activatePackage(RPGDialogEditor);
