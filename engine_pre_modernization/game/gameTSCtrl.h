//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMETSCTRL_H_
#define _GAMETSCTRL_H_

#ifndef _DGL_H_
#include "dgl/dgl.h"
#endif
#ifndef _GAME_H_
#include "game/game.h"
#endif
#ifndef _GUITSCONTROL_H_
#include "gui/core/guiTSControl.h"
#endif

class ProjectileData;
class GameBase;

//----------------------------------------------------------------------------
class GameTSCtrl : public GuiTSCtrl
{
private:
   typedef GuiTSCtrl Parent;

   // object selection additions start
   Point3F mMouse3DVec;
   Point3F mMouse3DPos;
   // object selection additions end

public:
   GameTSCtrl();

   bool processCameraQuery(CameraQuery *query);
   void renderWorld(const RectI &updateRect);

   void onMouseMove(const GuiEvent &evt);
   void onRender(Point2I offset, const RectI &updateRect);

   // object selection additions start
   void onMouseDown(const GuiEvent &evt); 
   //left-mouse click   
   Point3F getMouse3DVec() {return mMouse3DVec;};   
   Point3F getMouse3DPos() {return mMouse3DPos;}; 
   // object selection additions end

   DECLARE_CONOBJECT(GameTSCtrl);
};

#endif
