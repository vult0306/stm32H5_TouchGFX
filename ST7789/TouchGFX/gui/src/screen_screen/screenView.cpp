#include <gui/screen_screen/screenView.hpp>

// Both images are 120x120 on a 240x240 screen, so the waypoints are the four
// corners. Consecutive waypoints differ in one axis only, which turns each table
// into a single-direction leg per step. The two tables run the corners in
// opposite order, so the images never occupy the same corner at the same time.
const screenView::Waypoint screenView::image1Path[screenView::NUMBER_OF_WAYPOINTS] =
{
    { 120,   0 }, // right
    { 120, 120 }, // down
    {   0, 120 }, // left
    {   0,   0 }  // up, back to start
};

const screenView::Waypoint screenView::image2Path[screenView::NUMBER_OF_WAYPOINTS] =
{
    {   0, 120 }, // left
    {   0,   0 }, // up
    { 120,   0 }, // right
    { 120, 120 }  // down, back to start
};

screenView::screenView() :
    moveEndedCallback(this, &screenView::moveEndedHandler),
    image1Waypoint(0),
    image2Waypoint(0)
{

}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();

    scalableImage1.setMoveAnimationEndedAction(moveEndedCallback);
    scalableImage2.setMoveAnimationEndedAction(moveEndedCallback);

    startNextMove(scalableImage1, image1Path, image1Waypoint);
    startNextMove(scalableImage2, image2Path, image2Waypoint);
}

void screenView::tearDownScreen()
{
    scalableImage1.cancelMoveAnimation();
    scalableImage1.clearMoveAnimationEndedAction();
    scalableImage2.cancelMoveAnimation();
    scalableImage2.clearMoveAnimationEndedAction();

    screenViewBase::tearDownScreen();
}

void screenView::startNextMove(MovableImage& image, const Waypoint* path, uint8_t waypoint)
{
    image.startMoveAnimation(path[waypoint].x, path[waypoint].y, MOVE_DURATION,
                             touchgfx::EasingEquations::linearEaseIn,
                             touchgfx::EasingEquations::linearEaseIn);
}

void screenView::moveEndedHandler(const MovableImage& src)
{
    if (&src == &scalableImage1)
    {
        image1Waypoint = (image1Waypoint + 1) % NUMBER_OF_WAYPOINTS;
        startNextMove(scalableImage1, image1Path, image1Waypoint);
    }
    else if (&src == &scalableImage2)
    {
        image2Waypoint = (image2Waypoint + 1) % NUMBER_OF_WAYPOINTS;
        startNextMove(scalableImage2, image2Path, image2Waypoint);
    }
}
