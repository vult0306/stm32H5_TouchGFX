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
    // never fires. Drive the move animation from code instead, ping-ponging the
    // image between x = 0 and x = 120.
    touchgfx::Callback<screenView, const touchgfx::MoveAnimator<touchgfx::ScalableImage>&> moveEndedCallback;
    void moveEndedHandler(const touchgfx::MoveAnimator<touchgfx::ScalableImage>& src);
    void startNextMove();

    bool movingRight;
};

#endif // SCREENVIEW_HPP
