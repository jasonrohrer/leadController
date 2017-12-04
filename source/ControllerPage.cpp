#include "ControllerPage.h"

#include "musicPlayer.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/game/drawUtils.h"



extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;





ControllerPage::ControllerPage() {
    mLastMouseX = -1000;
    mLastMouseY = -1000;
    }



ControllerPage::~ControllerPage() {
    
    }






void ControllerPage::actionPerformed( GUIComponent *inTarget ) {
    }


int numBoxesX = 5;
int numBoxesY = 16;

static doublePair gridBottomPos = { -80, -300 };
static int boxR = 20;

    
void ControllerPage::getCurrentBox( int *outX, int *outY ) {
    
    if( mLastMouseX < gridBottomPos.x - boxR ||
        mLastMouseY < gridBottomPos.y - boxR ) {
        
        *outX = -1;
        *outY = -1;
        return;
        }

    *outX = (int)( ( boxR + mLastMouseX - gridBottomPos.x ) / ( boxR * 2 ) );

    *outY = (int)( ( boxR + mLastMouseY - gridBottomPos.y ) / ( boxR * 2 ) );

    if( *outX >= numBoxesX ||
        *outY >= numBoxesY ) {
        *outX = -1;
        *outY = -1;
        }
    }



void ControllerPage::draw( doublePair inViewCenter, 
                           double inViewSize ) {
    
    //doublePair pos = { 0, 200 };
    
    // no message for now
    //drawMessage( "", pos );
    
    
    int curX, curY;
    getCurrentBox( &curX, &curY );


    for( int y=0; y<numBoxesY; y++ ) {
        for( int x=0; x<numBoxesX; x++ ) {
            
            if( x == curX && y == curY ) {
                setDrawColor( 1, 0, 0, 1 );
                }
            else {
                setDrawColor( 1, 1, 1, 1 );
                }
            

            doublePair pos = gridBottomPos;
            
            pos.y += y * 2 * boxR;
            pos.x += x * 2 * boxR;
            

            drawSquare( pos, boxR - 2 );
            }
        }
    }



void ControllerPage::makeActive( char inFresh ) {
    
    }



void ControllerPage::pointerMove( float inX, float inY ) {
    mLastMouseX = inX;
    mLastMouseY = inY;

    int x, y;
    getCurrentBox( &x, &y );
    
    setButtonPressed( x, y );
    }

