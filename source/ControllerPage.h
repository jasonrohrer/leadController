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
        
    protected:

        SpriteButton *mButtonGrid[16][5];
    };
