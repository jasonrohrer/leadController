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


int numBoxesX = 16;
int numBoxesY = 5;

static doublePair gridBottomPos = { -300, -160 };
static int boxRY = 40;
static int boxRX = 20;

    
void ControllerPage::getCurrentBox( int *outX, int *outY ) {
    
    if( mLastMouseX < gridBottomPos.x - boxRX ||
        mLastMouseY < gridBottomPos.y - boxRY ) {
        
        *outX = -1;
        *outY = -1;
        return;
        }

    *outX = (int)( ( boxRX + mLastMouseX - gridBottomPos.x ) / ( boxRX * 2 ) );

    *outY = (int)( ( boxRY + mLastMouseY - gridBottomPos.y ) / ( boxRY * 2 ) );

    if( *outX >= numBoxesX ||
        *outY >= numBoxesY ) {
        *outX = -1;
        *outY = -1;
        }
    }


int bloomX = -1;
int bloomY = -1;

float bloomProgress = 0;


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
                setDrawColor( 1, 1, 1, 0.5 );
                }
            

            doublePair pos = gridBottomPos;
            
            pos.y += y * 2 * boxRY;
            pos.x += x * 2 * boxRX;
            

            drawRect( pos, boxRX - 2, boxRY - 2 );
            }
        }


    int lastX, lastY;
    getLastCellPlayed( &lastX, &lastY );
    
    if( lastX != -1 && lastY != -1 ) {
        bloomX = lastY;
        bloomY = 4 - lastX;
        bloomProgress = 0;
        }


    if( bloomX != -1 && bloomY != -1 ) {
        doublePair pos = gridBottomPos;
        
        pos.y += bloomY * 2 * boxRY;
        pos.x += bloomX * 2 * boxRX;

        setDrawColor( 1, 1, 0, 1 - bloomProgress );
        
        double bloomScale = 2 * bloomProgress + 1;
        
        drawRect( pos, 
                  bloomScale * ( boxRX - 2 ), 
                          bloomScale * ( boxRY - 2 ) );
        
        bloomProgress += 0.1;
        
        if( bloomProgress >= 1 ) {
            bloomX = -1;
            bloomY = -1;
            bloomProgress = 0;
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
    
    setButtonPressed( 4 - y, x );
    }

