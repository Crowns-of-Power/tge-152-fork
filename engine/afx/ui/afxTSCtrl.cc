
//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//
// Arcane-FX
// Copyright (C) Faust Logic, Inc.
//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//
// Some of the object selection code in this file is based on functionality described
// in the following resource:
//
// Object Selection in Torque by Dave Myers 
//   http://www.garagegames.com/index.php?sec=mg&mod=resource&page=view&qid=7335
//
//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

#include "afx/arcaneFX.h"

#include "gui/core/guiCanvas.h"
#include "afx/ui/afxTSCtrl.h"
#include "console/consoleTypes.h"
#include "game/projectile.h"
#include "game/gameBase.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "game/player.h"

#include "afx/afxSpellBook.h"

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

IMPLEMENT_CONOBJECT(afxTSCtrl);

afxTSCtrl::afxTSCtrl()
{
  mMouse3DVec.zero();
  mMouse3DPos.zero();
  mouse_dn_timestamp = 0;
  spellbook = NULL;
}

bool afxTSCtrl::processCameraQuery(CameraQuery *camq)
{
  GameUpdateCameraFov();
  return GameProcessCameraQuery(camq);
}

void afxTSCtrl::renderWorld(const RectI &updateRect)
{
  GameRenderWorld();
#if defined(TGEA_ENGINE)
  GFX->setClipRect(updateRect);
#else
  dglSetClipRect(updateRect);
#endif
}

void afxTSCtrl::onRender(Point2I offset, const RectI &updateRect)
{
  GameConnection* con = GameConnection::getConnectionToServer();
  bool skipRender = (!con || 
                     (con->getWhiteOut() >= 1.f) || 
                     (con->getDamageFlash() >= 1.f) || 
                     (con->getBlackOut() >= 1.f));

  if (!skipRender)
    Parent::onRender(offset, updateRect);

#if defined(TGEA_ENGINE)
  GFX->setViewport(updateRect);
#else
  dglSetViewport(updateRect);
#endif

#if !defined(TGEA_ENGINE)
  CameraQuery camq;
  if (GameProcessCameraQuery(&camq))
    GameRenderFilters(camq);
#endif
}

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

void afxTSCtrl::onMouseDown(const GuiEvent &evt)
{   
	raycastMouse(evt);	// DARREN MOD: mouse click movement, sometime, mouse dun move camera move
	Con::executef(this, 1, "onMouseDown");
	// direct calling method, faster but might break current code
	//GameConnection::getConnectionToServer()->setPreSelectedObjFromRollover();

  //Con::printf("#### afxTSCtrl::onLeftMouseDown() ####");

  // save a timestamp so we can measure how long the button is down
  mouse_dn_timestamp = Platform::getRealMilliseconds();

  // clear button down status because the ActionMap is going to capture
  // the mouse and the button up will never happen
  Canvas->clearMouseButtonDown();

  // indicate that processing of event should continue (pass down to ActionMap)
  Canvas->setConsumeLastInputEvent(false);
}

void afxTSCtrl::onMouseUp(const GuiEvent& evt)
{
	//Con::printf("#### afxTSCtrl::onLeftMouseUp() ####");
	raycastMouse(evt);	// DARREN MOD: mouse click movement, fix player not moves to mouse up location
	Con::executef(this, 1, "onMouseUp");
	// direct calling method, faster but might break current code
	//GameConnection::getConnectionToServer()->setSelectedObjFromPreSelected();
}

void afxTSCtrl::onRightMouseDown(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onRightMouseDown() ####");

  // clear right button down status because the ActionMap is going to capture
  // the mouse and the right button up will never happen
  Canvas->clearMouseRightButtonDown();

  // indicate that processing of event should continue (pass down to ActionMap)
  Canvas->setConsumeLastInputEvent(false);
}

void afxTSCtrl::onRightMouseUp(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onRightMouseUp() ####");
}

void afxTSCtrl::onMouseMove(const GuiEvent& evt)
{
	// DARREN MOD: mouse click movement, 
	// can use this to display mouseover character info and change cursor
	Con::executef(this, 2, "onMouseMove",raycastMouse(evt));	
}

void afxTSCtrl::onMouseEnter(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onMouseEnter() ####");
}

void afxTSCtrl::onMouseDragged(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onMouseDragged() ####");
}


void afxTSCtrl::onMouseLeave(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onMouseLeave() ####");
}

bool afxTSCtrl::onMouseWheelUp(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onMouseWheelUp() ####");
  Con::executef(this, 1, "onMouseWheelUp");
  return true;
}

bool afxTSCtrl::onMouseWheelDown(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onMouseWheelDown() ####");
  Con::executef(this, 1, "onMouseWheelDown");
  return true;
}


void afxTSCtrl::onRightMouseDragged(const GuiEvent& evt)
{
  //Con::printf("#### afxTSCtrl::onRightMouseDragged() ####");
}

// DARREN MOD: mouse click movement, sometime, mouse dun move camera move
S32 afxTSCtrl::raycastMouse(const GuiEvent &evt)
{
  MatrixF cam_xfm;
  Point3F dummy_pt;
  if (GameGetCameraTransform(&cam_xfm, &dummy_pt))    
  {      
    // get cam pos 
    Point3F cameraPoint; cam_xfm.getColumn(3,&cameraPoint); 

    // construct 3D screen point from mouse coords 
    Point3F screen_pt(evt.mousePoint.x, evt.mousePoint.y, -1);  

    // convert screen point to world point
    Point3F world_pt;      
    if (unproject(screen_pt, &world_pt))       
    {         
      Point3F mouseVec = world_pt - cameraPoint;         
      mouseVec.normalizeSafe();    
      
      mMouse3DPos = cameraPoint;
      mMouse3DVec = mouseVec;

      F32 selectRange = arcaneFX::sTargetSelectionRange;
      Point3F mouseScaled = mouseVec*selectRange;
      Point3F rangeEnd = cameraPoint + mouseScaled;

      return arcaneFX::rolloverRayCast(cameraPoint, rangeEnd, arcaneFX::sTargetSelectionMask, arcaneFX::sDestinationMask);
    }   
  }
  return 0;
}

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

void afxTSCtrl::setSpellBook(afxSpellBook* book)
{
  if (book != spellbook)
  {
    spellbook = book;
    Con::executef(this, 2, "onSpellbookChange", (spellbook) ? spellbook->scriptThis() : "");
  }
}

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

ConsoleMethod(afxTSCtrl, setSpellBook, void, 3, 3, "setSpellBook(book)")
{
  object->setSpellBook(dynamic_cast<afxSpellBook*>(Sim::findObject(argv[2])));
}

ConsoleMethod(afxTSCtrl, getMouse3DVec, const char *, 2, 2, "")
{
  char* rbuf = Con::getReturnBuffer(256);
  const Point3F& vec = object->getMouse3DVec();
  dSprintf(rbuf, 256, "%g %g %g", vec.x, vec.y, vec.z);
  return rbuf;
}

ConsoleMethod(afxTSCtrl, getMouse3DPos, const char *, 2, 2, "")
{
  char* rbuf = Con::getReturnBuffer(256);
  const Point3F& pos = object->getMouse3DPos();
  dSprintf(rbuf, 256, "%g %g %g", pos.x, pos.y, pos.z);
  return rbuf;
}

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//