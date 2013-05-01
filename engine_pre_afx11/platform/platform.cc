#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include  "platform/platformMutex.h"

S32 sgBackgroundSleepTime = 3000;
//S32 sgForegroundSleepTime = 1;
S32 sgTimeManagerProcessInterval = 0;

void Platform::initConsole()
{
   Con::addVariable("pref::backgroundSleepTime", TypeS32, &sgBackgroundSleepTime);
   //Con::addVariable("pref::foregroundSleepTime", TypeS32, &sgForegroundSleepTime); // DARREN MOD: reduce cpu usage?
   Con::addVariable("pref::timeManagerProcessInterval", TypeS32, &sgTimeManagerProcessInterval);
}

S32 Platform::getBackgroundSleepTime()
{
   return sgBackgroundSleepTime;
}
/*
S32 Platform::getForegroundSleepTime()
{
   return sgForegroundSleepTime;
}
*/

//------------------------------------------------------------------
// TGE Modernization Kit
//------------------------------------------------------------------

ConsoleFunction(getProcessorName, const char*, 1, 1, "")
{
   return Platform::SystemInfo.processor.name;
}

ConsoleFunction(getProcessorSpeed, S32, 1, 1, "")
{
   return Platform::SystemInfo.processor.mhz;
}

//------------------------------------------------------------------
// TGE Modernization Kit
//------------------------------------------------------------------