// Torque Input Map File
if (isObject(moveMap)) moveMap.delete();
new ActionMap(moveMap);
moveMap.bind(keyboard, "f2", showPlayerList);
moveMap.bind(keyboard, "f5", toggleParticleEditor);
moveMap.bind(keyboard, "w", moveforward);
moveMap.bind(keyboard, "s", movebackward);
moveMap.bind(keyboard, "space", jump);
moveMap.bind(keyboard, "a", turnLeft);
moveMap.bind(keyboard, "d", turnRight);
moveMap.bind(keyboard, "r", setZoomFOV);
moveMap.bind(keyboard, "e", toggleZoom);
moveMap.bind(keyboard, "m", toggleMouseLook);
moveMap.bind(keyboard, "z", toggleFreeLook);
moveMap.bind(keyboard, "tab", toggleFirstPerson);
moveMap.bind(keyboard, "alt c", toggleCamera);
moveMap.bind(keyboard, "ctrl w", celebrationWave);
moveMap.bind(keyboard, "ctrl s", celebrationSalute);
moveMap.bind(keyboard, "ctrl k", suicide);
moveMap.bindCmd(keyboard, "1", "commandToServer(\'use\',\"Crossbow\");", "");
moveMap.bind(keyboard, "u", toggleMessageHud);
moveMap.bind(keyboard, "pageup", pageMessageHudUp);
moveMap.bind(keyboard, "pagedown", pageMessageHudDown);
moveMap.bind(keyboard, "p", resizeMessageHud);
moveMap.bind(keyboard, "f8", dropCameraAtPlayer);
moveMap.bind(keyboard, "f7", dropPlayerAtCamera);
moveMap.bind(keyboard, "ctrl o", bringUpOptions);
moveMap.bind(keyboard, "q", TalkTo);
moveMap.bindCmd(keyboard, "c", "", "MapHudVisibility();");
moveMap.bindCmd(keyboard, "left", "azimuthLeft();", "cancelCameraAzimuth();");
moveMap.bindCmd(keyboard, "right", "azimuthRight();", "cancelCameraAzimuth();");
moveMap.bindCmd(keyboard, "up", "declineUp();", "cancelCameraDecline();");
moveMap.bindCmd(keyboard, "down", "declineDown();", "cancelCameraDecline();");
moveMap.bind(mouse0, "zaxis", zoomCamera);