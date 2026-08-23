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
    // Board has no touch controller, so the Designer interactions (on button click)
    // never fire. Drive the move animations from code instead: both images walk the
    // four corners of the screen, in opposite directions.
    //   scalableImage1: starts top left,     right -> down -> left -> up
    //   scalableImage2: starts bottom right, left  -> up   -> right -> down
    static const uint8_t NUMBER_OF_WAYPOINTS = 4;
    static const uint16_t MOVE_DURATION = 60; // ticks (~1 s at 60 Hz)

    struct Waypoint
    {
        int16_t x;
        int16_t y;
    };
    static const Waypoint image1Path[NUMBER_OF_WAYPOINTS];
    static const Waypoint image2Path[NUMBER_OF_WAYPOINTS];

    typedef touchgfx::MoveAnimator<touchgfx::ScalableImage> MovableImage;

    touchgfx::Callback<screenView, const MovableImage&> moveEndedCallback;
    void moveEndedHandler(const MovableImage& src);
    void startNextMove(MovableImage& image, const Waypoint* path, uint8_t waypoint);

    uint8_t image1Waypoint;
    uint8_t image2Waypoint;
};

#endif // SCREENVIEW_HPP
