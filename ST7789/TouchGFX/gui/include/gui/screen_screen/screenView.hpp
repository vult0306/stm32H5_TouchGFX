#ifndef SCREENVIEW_HPP
#define SCREENVIEW_HPP

#include <gui_generated/screen_screen/screenViewBase.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

class screenView : public screenViewBase
{
public:
    screenView();
    virtual ~screenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
    // Board has no touch controller, so the Designer interaction (on button click)
    // never fires. Drive the move animation from code instead: the image walks the
    // four corners of the screen, right -> down -> left -> up, and repeats.
    static const uint8_t NUMBER_OF_WAYPOINTS = 4;
    static const uint16_t MOVE_DURATION = 60; // ticks (~1 s at 60 Hz)

    struct Waypoint
    {
        int16_t x;
        int16_t y;
    };
    static const Waypoint waypoints[NUMBER_OF_WAYPOINTS];

    touchgfx::Callback<screenView, const touchgfx::MoveAnimator<touchgfx::ScalableImage>&> moveEndedCallback;
    void moveEndedHandler(const touchgfx::MoveAnimator<touchgfx::ScalableImage>& src);
    void startNextMove();

    uint8_t currentWaypoint;
};

#endif // SCREENVIEW_HPP
