#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

//#define USE_MALLINFO

#ifdef USE_MALLINFO
#include <malloc.h>
#endif


#include "minorGems/graphics/Color.h"





#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/io/file/File.h"

#include "minorGems/system/Time.h"


#include "minorGems/util/log/AppLog.h"



#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"



#include "musicPlayer.h"
#include "ControllerPage.h"



ControllerPage *controllerPage;


GamePage *currentGamePage = NULL;




// position of view in world
doublePair lastScreenViewCenter = {0, 0 };



// world width of one view
double viewWidth = 1280;
double viewHeight = 720;


// this is the desired visible width
// if our screen is wider than this (wider than 16:9 aspect ratio)
// then we will put letterbox bars on the sides
// Usually, if screen is not 16:9, it will be taller, not wider,
// and we will put letterbox bars on the top and bottom 
const double visibleViewWidth = viewWidth;



// fraction of viewWidth visible vertically (aspect ratio)
double viewHeightFraction;

int screenW, screenH;

char initDone = false;



double frameRateFactor = 1;
int baseFramesPerSecond = 60;


char firstDrawFrameCalled = false;






char doesOverrideGameImageSize() {
    return true;
    }



void getGameImageSize( int *outWidth, int *outHeight ) {
    *outWidth = (int)viewWidth;
    *outHeight = (int)viewHeight;
    }


char shouldNativeScreenResolutionBeUsed() {
    return true;
    }


char isNonIntegerScalingAllowed() {
    return true;
    }


const char *getWindowTitle() {
    return "Lead Controller";
    }


const char *getAppName() {
    return "LeadController";
    }

const char *getLinuxAppName() {
    // no dir-name conflict here because we're using all caps for app name
    return "LeadControllerApp";
    }



const char *getFontTGAFileName() {
    return "font_32_64.tga";
    }


char isDemoMode() {
    return false;
    }


const char *getDemoCodeSharedSecret() {
    return "fundamental_right";
    }


const char *getDemoCodeServerURL() {
    return "http://FIXME/demoServer/server.php";
    }



char gamePlayingBack = false;


Font *mainFont;









static float pauseScreenFade = 0;










#define SETTINGS_HASH_SALT "another_loss"




char *getCustomRecordedGameData() {    
    
    char * result = stringDuplicate( "" );

    return result;
    }



char showMouseDuringPlayback() {
    // since we rely on the system mouse pointer during the game (and don't
    // draw our own pointer), we need to see the recorded pointer position
    // to make sense of game playback
    return true;
    }



char *getHashSalt() {
    return stringDuplicate( SETTINGS_HASH_SALT );
    }




void initDrawString( int inWidth, int inHeight ) {

    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );
    
    mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16 );
    mainFont->setMinimumPositionPrecision( 1 );

    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;
    
    if( viewHeightFraction < 9.0 / 16.0 ) {
        // weird, wider than 16:9 aspect ratio
        
        viewWidth = viewHeight / viewHeightFraction;
        }
    

    setViewSize( viewWidth );
    setLetterbox( visibleViewWidth, viewHeight );
    }


void freeDrawString() {
    delete mainFont;
    }



void initFrameDrawer( int inWidth, int inHeight, int inTargetFrameRate,
                      const char *inCustomRecordedGameData,
                      char inPlayingBack ) {

    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );
    
    gamePlayingBack = inPlayingBack;
    
    screenW = inWidth;
    screenH = inHeight;
    
    if( inTargetFrameRate != baseFramesPerSecond ) {
        frameRateFactor = 
            (double)baseFramesPerSecond / (double)inTargetFrameRate;
        
        }
    
    


    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;
    

    if( viewHeightFraction < 9.0 / 16.0 ) {
        // weird, wider than 16:9 aspect ratio
        
        viewWidth = viewHeight / viewHeightFraction;
        }
    
    setViewSize( viewWidth );
    setLetterbox( visibleViewWidth, viewHeight );


    
    

    

    setCursorVisible( true );
    grabInput( false );
    
    // world coordinates
    setMouseReportingMode( true );
    
    
    initMusicPlayer();
    


    setSoundLoudness( 1.0 );
    setSoundPlaying( true );

    

    controllerPage = new ControllerPage;

    currentGamePage = controllerPage;

    currentGamePage->base_makeActive( true );

    initDone = true;
    }




void freeFrameDrawer() {
    
    freeMusicPlayer();
    
    delete controllerPage;
    }





    


// draw code separated from updates
// some updates are still embedded in draw code, so pass a switch to 
// turn them off
static void drawFrameNoUpdate( char inUpdate );




static void drawPauseScreen() {

    double viewHeight = viewHeightFraction * viewWidth;

    setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
        
    drawSquare( lastScreenViewCenter, 1.05 * ( viewHeight / 3 ) );
        

    setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
        
    drawSquare( lastScreenViewCenter, viewHeight / 3 );
        

    setDrawColor( 1, 1, 1, pauseScreenFade );

    doublePair messagePos = lastScreenViewCenter;

    messagePos.y += 4.5  * (viewHeight / 15);

    mainFont->drawString( translate( "pauseMessage1" ), 
                           messagePos, alignCenter );
        
    messagePos.y -= 1.25 * (viewHeight / 15);
    mainFont->drawString( translate( "pauseMessage2" ), 
                           messagePos, alignCenter );    

        
        

    setDrawColor( 1, 1, 1, pauseScreenFade );

    messagePos = lastScreenViewCenter;

    messagePos.y -= 3.75 * ( viewHeight / 15 );
    //mainFont->drawString( translate( "pauseMessage3" ), 
    //                      messagePos, alignCenter );

    messagePos.y -= 0.625 * (viewHeight / 15);

    const char* quitMessageKey = "pauseMessage3";
    
    if( isQuittingBlocked() ) {
        quitMessageKey = "pauseMessage3b";
        }

    mainFont->drawString( translate( quitMessageKey ), 
                          messagePos, alignCenter );

    }












void drawFrame( char inUpdate ) {    


    if( !inUpdate ) {
        
    
        if( currentGamePage != NULL ) {
            currentGamePage->base_step();
            }
        wakeUpPauseFrameRate();


        drawFrameNoUpdate( true );
            
        drawPauseScreen();
        


        // fade in pause screen
        if( pauseScreenFade < 1 ) {
            pauseScreenFade += ( 1.0 / 30 ) * frameRateFactor;
        
            if( pauseScreenFade > 1 ) {
                pauseScreenFade = 1;
                }
            }
        

        return;
        }


    // not paused


    // fade pause screen out
    if( pauseScreenFade > 0 ) {
        pauseScreenFade -= ( 1.0 / 30 ) * frameRateFactor;
        
        if( pauseScreenFade < 0 ) {
            pauseScreenFade = 0;
            }
        }    
    
    

    if( !firstDrawFrameCalled ) {
        
        // do final init step... stuff that shouldn't be done until
        // we have control of screen

        firstDrawFrameCalled = true;
        }




    // updates here

    if( currentGamePage != NULL ) {
        currentGamePage->base_step();
        

        if( currentGamePage == controllerPage ) {
            
            } 
        }
    


    // now draw stuff AFTER all updates
    drawFrameNoUpdate( true );






    // draw tail end of pause screen, if it is still visible
    if( pauseScreenFade > 0 ) {
        drawPauseScreen();
        }
    }




void drawFrameNoUpdate( char inUpdate ) {
    if( currentGamePage != NULL ) {
        currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
        }
    }







void pointerMove( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerMove( inX, inY );
        }
    }





void pointerDown( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDown( inX, inY );
        }
    }



void pointerDrag( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDrag( inX, inY );
        }
    }



void pointerUp( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerUp( inX, inY );
        }
    }







void keyDown( unsigned char inASCII ) {

    // taking screen shot is ALWAYS possible
    if( inASCII == '=' ) {    
        saveScreenShot( "screen" );
        }

    
    if( isPaused() ) {
        // block general keyboard control during pause


        switch( inASCII ) {
            case 13:  // enter
                // unpause
                pauseGame();
                break;
            }
        
        }
    

    if( currentGamePage != NULL ) {
        currentGamePage->base_keyDown( inASCII );
        }


    
    switch( inASCII ) {
        case 'm':
        case 'M': {
#ifdef USE_MALLINFO
            struct mallinfo meminfo = mallinfo();
            printf( "Mem alloc: %d\n",
                    meminfo.uordblks / 1024 );
#endif
            }
            break;
        }
    }



void keyUp( unsigned char inASCII ) {
    
    if( ! isPaused() ) {

        if( currentGamePage != NULL ) {
            currentGamePage->base_keyUp( inASCII );
            }
        }

    }







void specialKeyDown( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyDown( inKey );
        }
	}



void specialKeyUp( int inKey ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyUp( inKey );
        }
	} 




char getUsesSound() {
    
    return true;
    }









void drawString( const char *inString, char inForceCenter ) {
    
    setDrawColor( 1, 1, 1, 0.75 );

    doublePair messagePos = lastScreenViewCenter;

    TextAlignment align = alignCenter;
    
    if( initDone && !inForceCenter ) {
        // transparent message
        setDrawColor( 1, 1, 1, 0.75 );

        // stick messages in corner
        messagePos.x -= viewWidth / 2;
        
        messagePos.x +=  20;
    

    
        messagePos.y += (viewWidth * viewHeightFraction) /  2;
    
        messagePos.y -= 32;

        align = alignLeft;
        }
    else {
        // fully opaque message
        setDrawColor( 1, 1, 1, 1 );

        // leave centered
        }
    

    int numLines;
    
    char **lines = split( inString, "\n", &numLines );
    
    for( int i=0; i<numLines; i++ ) {
        

        mainFont->drawString( lines[i], messagePos, align );
        messagePos.y -= 32;
        
        delete [] lines[i];
        }
    delete [] lines;
    }













