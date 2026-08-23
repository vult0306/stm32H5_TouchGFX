#include <gui/screen_screen/screenView.hpp>

screenView::screenView() :
    moveEndedCallback(this, &screenView::moveEndedHandler),
    movingRight(true)
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
    const int16_t endX = movingRight ? 120 : 0;
    scalableImage1.startMoveAnimation(endX, 0, 60,
                                      touchgfx::EasingEquations::linearEaseIn,
                                      touchgfx::EasingEquations::linearEaseIn);
}

void screenView::moveEndedHandler(const touchgfx::MoveAnimator<touchgfx::ScalableImage>& src)
{
    if (&src == &scalableImage1)
    {
        movingRight = !movingRight;
        startNextMove();
    }
}
