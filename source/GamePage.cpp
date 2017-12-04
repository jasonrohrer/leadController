#include "GamePage.h"

#include "message.h"



#include "minorGems/util/stringUtils.h"
#include "minorGems/game/game.h"



int GamePage::sPageCount = 0;

SpriteHandle GamePage::sWaitingSprites[3] = { NULL, NULL, NULL };
SpriteHandle GamePage::sResponseWarningSprite = NULL;

int GamePage::sCurrentWaitingSprite = 0;
int GamePage::sLastWaitingSprite = -1;
int GamePage::sWaitingSpriteDirection = 1;
double GamePage::sCurrentWaitingSpriteFade = 0;

char GamePage::sResponseWarningShowing = false;
doublePair GamePage::sResponseWarningPosition = { 0, 0 };


double GamePage::sWaitingFade = 0;
char GamePage::sWaiting = false;
char GamePage::sShowWaitingWarningOnly = false;

char GamePage::sShutdownPendingWarning = false;





GamePage::GamePage()
        : PageComponent( 0, 0 ),
          mStatusError( false ),
          mStatusMessageKey( NULL ),
          mStatusMessage( NULL ),
          mSkipDrawingSubComponents( false ),
          mTip( NULL ),
          mLastTip( NULL ),
          mLastTipFade( 1 ),
          mTipAtTopOfScreen( false ),
          mStatusAtTopOfScreen( false ),
          mSignal( NULL ),
          mResponseWarningTipShowing( false ) {

    sPageCount++;
    }




GamePage::~GamePage() {
    if( mStatusMessage != NULL ) {
        delete [] mStatusMessage;
        }
    if( mTip != NULL ) {
        delete [] mTip;
        }
    if( mLastTip != NULL ) {
        delete [] mLastTip;
        }
    
    clearSignal();
    
    
    sPageCount--;
    }




void GamePage::skipDrawingSubComponents( char inSkip ) {
    mSkipDrawingSubComponents = inSkip;
    }



void GamePage::setStatus( const char *inStatusMessageKey, char inError ) {
    mStatusMessageKey = inStatusMessageKey;
    mStatusError = inError;

    if( mStatusMessage != NULL ) {
        delete [] mStatusMessage;
        mStatusMessage = NULL;
        }
    }



void GamePage::setStatusDirect( const char *inStatusMessage, char inError ) {
    if( mStatusMessage != NULL ) {
        delete [] mStatusMessage;
        mStatusMessage = NULL;
        }
    
    if( inStatusMessage != NULL ) {
        mStatusMessage = stringDuplicate( inStatusMessage );
        
        mStatusMessageKey = NULL;
        }
    
    mStatusError = inError;
    }



char GamePage::isStatusShowing() {
    return ( mStatusMessage != NULL || mStatusMessageKey != NULL );
    }



void GamePage::setToolTip( const char *inTip ) {
    if( mTip != NULL && inTip == NULL ) {
        // tip disappearing, save it as mLastTip
        if( mLastTip != NULL ) {
            delete [] mLastTip;
            }
        mLastTip = mTip;
        mLastTipFade = 1.0;
        }
    else if( mTip != NULL ) {
        delete [] mTip;
        }
        
    if( inTip != NULL ) {
        mTip = stringDuplicate( inTip );
        }
    else {
        mTip = NULL;
        }

    mResponseWarningTipShowing = false;
    }



void GamePage::clearToolTip( const char *inTipToClear ) {
    if( mTip != NULL ) {
        if( strcmp( mTip, inTipToClear ) == 0 ) {

            // tip disappearing, save it as mLastTip
            if( mLastTip != NULL ) {
                delete [] mLastTip;
                }
            mLastTip = mTip;
            mLastTipFade = 1.0;
            
            mTip = NULL;
            }
        }
    }



void GamePage::setTipPosition( char inTop ) {
    mTipAtTopOfScreen = inTop;
    }


void GamePage::setStatusPositiion( char inTop ) {
    mStatusAtTopOfScreen = inTop;
    }



void GamePage::base_draw( doublePair inViewCenter, 
                          double inViewSize ){

    if( sShutdownPendingWarning ) {
        // skip drawing current page and draw warning instead

        doublePair labelPos = { 0, 0 };
        
        drawMessage( "shutdownPendingWarning", labelPos );
        
        return;
        }

    
    drawUnderComponents( inViewCenter, inViewSize );

    if( !mSkipDrawingSubComponents ) {
        PageComponent::base_draw( inViewCenter, inViewSize );
        }
    
    char statusDrawn = false;
    
    if( mStatusMessageKey != NULL ) {
        doublePair labelPos = { 0, -280 };
        
        if( mStatusAtTopOfScreen ) {
            labelPos.y *= -1;
            }
        
        drawMessage( mStatusMessageKey, labelPos, mStatusError );
        statusDrawn = true;
        }
    else if( mStatusMessage != NULL ) {
        doublePair labelPos = { 0, -280 };
        
        if( mStatusAtTopOfScreen ) {
            labelPos.y *= -1;
            }
        
        drawMessage( mStatusMessage, labelPos, mStatusError );
        statusDrawn = true;
        }


    // skip drawing tip if status showing
    if( ! statusDrawn ) {
        
        doublePair tipPosition = { 0, -280 };
        
        if( mTipAtTopOfScreen ) {
            tipPosition.y *= -1;
            }
        
        
        if( mTip != NULL ) {
            drawMessage( mTip, tipPosition );
            }
        else if( mLastTip != NULL && mLastTipFade > 0 ) {
            drawMessage( mLastTip, tipPosition, false, mLastTipFade );
            }
        }
    
    
    draw( inViewCenter, inViewSize );
    }


extern double frameRateFactor;


void GamePage::base_step() {
    if( sShutdownPendingWarning ) {
        // skip stepping stuff so that game doesn't advance
        // while the user's view is obscured
        return;
        }
    
    PageComponent::base_step();


    mLastTipFade -= 0.025 * frameRateFactor;
    if( mLastTipFade < 0 ) {
        mLastTipFade = 0;
        }
    

    
        
    }



void GamePage::showShutdownPendingWarning() {
    sShutdownPendingWarning = true;
    setIgnoreEvents( true );
    }



void GamePage::setSignal( const char *inSignalName ) {
    clearSignal();
    mSignal = stringDuplicate( inSignalName );
    }



void GamePage::clearSignal() {
    if( mSignal != NULL ) {
        delete [] mSignal;
        }
    mSignal = NULL;
    }



char GamePage::checkSignal( const char *inSignalName ) {
    if( mSignal == NULL ) {
        return false;
        }
    else if( strcmp( inSignalName, mSignal ) == 0 ) {
        return true;
        }
    else {
        return false;
        }
    }




void GamePage::base_keyDown( unsigned char inASCII ) {
    PageComponent::base_keyDown( inASCII );
    
    if( sShutdownPendingWarning && inASCII == ' ' ) {
        sShutdownPendingWarning = false;
        setIgnoreEvents( false );
        }
    }




void GamePage::base_makeActive( char inFresh ){
    if( inFresh ) {    
        for( int i=0; i<mComponents.size(); i++ ) {
            PageComponent *c = *( mComponents.getElement( i ) );
            
            c->base_clearState();
            }

        // don't show lingering tool tips from last time page was shown
        mLastTipFade = 0;
        if( mTip != NULL ) {
            delete [] mTip;
            mTip = NULL;
            }

        clearSignal();
        }
    

    makeActive( inFresh );
    }



void GamePage::base_makeNotActive(){
    for( int i=0; i<mComponents.size(); i++ ) {
        PageComponent *c = *( mComponents.getElement( i ) );
        
        c->base_clearState();
        }
    
    makeNotActive();
    }




void GamePage::setWaiting( char inWaiting, char inWarningOnly ) {
    sWaiting = inWaiting;
    sShowWaitingWarningOnly = inWarningOnly;
    
    if( sWaiting == false && mResponseWarningTipShowing ) {
        setToolTip( NULL );
        }
    }




void GamePage::pointerMove( float inX, float inY ) {
    }

    




