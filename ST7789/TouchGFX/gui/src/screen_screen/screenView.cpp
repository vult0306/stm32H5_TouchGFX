#include <gui/screen_screen/screenView.hpp>

// The image is 120x120 on a 240x240 screen, so the four waypoints are the four
// corners. Consecutive waypoints differ in one axis only, which gives the
// right -> down -> left -> up cycle.
const screenView::Waypoint screenView::waypoints[screenView::NUMBER_OF_WAYPOINTS] =
{
    { 120,   0 }, // right
    { 120, 120 }, // down
    {   0, 120 }, // left
    {   0,   0 }  // up, back to start
};

screenView::screenView() :
    moveEndedCallback(this, &screenView::moveEndedHandler),
    currentWaypoint(0)
{

}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();

    scalableImage1.setMoveAnimationEndedAction(moveEndedCallback);
    startNextMove();
}

void screenView::tearDownScreen()
{
    scalableImage1.cancelMoveAnimation();
    scalableImage1.clearMoveAnimationEndedAction();

    screenViewBase::tearDownScreen();
}

void screenView::startNextMove()
{
    const Waypoint& target = waypoints[currentWaypoint];
    scalableImage1.startMoveAnimation(target.x, target.y, MOVE_DURATION,
                                      touchgfx::EasingEquations::linearEaseIn,
                                      touchgfx::EasingEquations::linearEaseIn);
}

void screenView::moveEndedHandler(const touchgfx::MoveAnimator<touchgfx::ScalableImage>& src)
{
    if (&src == &scalableImage1)
    {
        currentWaypoint = (currentWaypoint + 1) % NUMBER_OF_WAYPOINTS;
        startNextMove();
    }
}
