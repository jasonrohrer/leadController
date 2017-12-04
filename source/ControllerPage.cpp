#include "ControllerPage.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"


extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;





ControllerPage::ControllerPage() {
    }



ControllerPage::~ControllerPage() {
    
    }






void ControllerPage::actionPerformed( GUIComponent *inTarget ) {
    }



void ControllerPage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    //doublePair pos = { 0, 200 };
    
    // no message for now
    //drawMessage( "", pos );
    }



void ControllerPage::makeActive( char inFresh ) {
    
    }
