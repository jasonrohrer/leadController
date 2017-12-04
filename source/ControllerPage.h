#include "GamePage.h"



#include "minorGems/ui/event/ActionListener.h"


#include "SpriteButton.h"


class ControllerPage : public GamePage, public ActionListener {
        
    public:
        ControllerPage();
        ~ControllerPage();
        

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void makeActive( char inFresh );
        
        virtual void pointerMove( float inX, float inY );

    protected:

        float mLastMouseX, mLastMouseY;

        void getCurrentBox( int *outX, int *outY );

    };
