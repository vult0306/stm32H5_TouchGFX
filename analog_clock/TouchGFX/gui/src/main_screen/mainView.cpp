#include <gui/main_screen/mainView.hpp>
#include "main.h"

extern "C" RTC_HandleTypeDef hrtc;

mainView::mainView()
    : lastSecond(0xFF)
{

}

void mainView::setupScreen()
{
    mainViewBase::setupScreen();
}

void mainView::tearDownScreen()
{
    mainViewBase::tearDownScreen();
}

void mainView::handleTickEvent()
{
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    /* GetDate must follow GetTime to unlock the RTC shadow registers */
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    /* Redraw only when the second actually changes: a full repaint of the
       clock is expensive on this part (no graphics accelerator). */
    if (time.Seconds != lastSecond)
    {
        lastSecond = time.Seconds;
        analogClock1.setTime24Hour(time.Hours, time.Minutes, time.Seconds);
    }
}
