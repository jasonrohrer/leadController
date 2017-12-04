#include "minorGems/game/game.h"

#include "Timbre.h"
#include "Envelope.h"

#include <stdlib.h>
#include <string.h>



static int pressedX, pressedY;

static int lastPlayX;


// 16 pitches
static Timbre *timbre;
// 5 note lengths
static Envelope *envelope;

// c
//double keyFrequency = 261.63;

// actually, can't support high notes in the key of c w/out round-off errors
// because this is a wavetable implementation, and when the tables get short,
// the errors get huge
// THIS, however, evenly divides our sample rate (22050)
// which means that we get perfect, whole-number wave tables at octaves
static double keyFrequency = 172.265625;


static int sampleRate;


// close to 120 bpm
// 16th note is 5512 samples
static int gridStepSamples = 5512;

static int numSamplesPassed = 0;

static int curNoteStepsleft = 0;

// pitch in timbre set
static int curNotePitch = -1;
// in grid steps
static int curNoteStepLength = -1;

static int curNoteSamplesPassed = 0;




// limit on n, based on Nyquist, when summing sine components
//int nLimit = (int)( sampleRate * M_PI );
// actually, this is way too many:  it takes forever to compute
// use a lower limit instead
// This produces fine results (almost perfect square wave)
#define nLimit 40

double sumCoefficients[nLimit];

void initSumCoefficients() {
    for( int n=1; n<nLimit; n++ ) {
        sumCoefficients[n] = 1.0/n;
        }
    }


// square wave with period of 2pi
double squareWave( double inT ) {
    double sum = 0;
    
    for( int n=1; n<nLimit; n+=2 ) {
        sum += sumCoefficients[n] * sin( n * inT );
        }
    return sum;
    }



// sawtooth wave with period of 2pi
double sawWave( double inT ) {
    double sum = 0;
    
    for( int n=1; n<nLimit; n++ ) {
        sum += sumCoefficients[n] * sin( n * inT );
        }
    return sum;
    }


double harmonicSaw( double inT ) {
    return
        1.0 * sawWave( inT ) 
        +
        0.5 * sawWave( 2 * inT )
        +
        0.25 * sawWave( 4 * inT );
    }


double harmonicSquare( double inT ) {
    return
        1.0 * squareWave( inT ) 
        +
        0.5 * squareWave( 2 * inT )
        +
        0.25 * squareWave( 4 * inT );
    }


// white noise, ignores inT
double whiteNoise( double inT ) {
    return 2.0 * ( rand() / (double)RAND_MAX ) - 1.0;
    }




#include "minorGems/sound/filters/coefficientFilters.h"


typedef struct BandPassFilterState {
        CoeffFilterState hpState;
        CoeffFilterState lpState;
    } BandPassFilterState;


BandPassFilterState initBandPass( double inCutoffFreq, double inRez ) {
    BandPassFilterState s;
    s.hpState = initHighPass( inCutoffFreq, sampleRate, inRez );
    s.lpState = initLowPass( inCutoffFreq, sampleRate, inRez );

    return s;
    }


double bandPassFilter( double inSample, BandPassFilterState *s ) {
    double hpSample = coeffFilter( inSample, &( s->hpState ) );

    return coeffFilter( hpSample, &( s->lpState ) );
    }


void resetBandPass( BandPassFilterState *s ) {
    resetCoeffFilter( &( s->hpState ) );
    resetCoeffFilter( &( s->lpState ) );
    }

CoeffFilterState sawFilterState;

// file for input into gnuplot to view filtered vs unfiltered waves
// plot "saw.txt" using 1:2 with lines, "saw.txt" using 1:3 with lines
//FILE *sawFile = fopen( "saw.txt", "w" );
// must init sawFilterState before calling
int tableCount = 0;
double filteredSawWave( double inT ) {
    if( inT == 0 ) {
        resetCoeffFilter( &sawFilterState );
        tableCount ++;
        }

    double plainSaw = harmonicSaw( inT ) + harmonicSquare( inT ) + 
        whiteNoise( inT );
    
    double filteredSaw = coeffFilter( plainSaw, &sawFilterState );
    
    //if( tableCount < 2 ) {
    //    fprintf( sawFile, "%f %f %f\n", inT, plainSaw, filteredSaw );
    //    }

    return filteredSaw;
    }




void initMusicPlayer() {
    pressedX = -1;
    pressedY = -1;
    lastPlayX = -1;
    
    sampleRate = getSampleRate();
    
    initSumCoefficients();

    setDefaultScale();
    

    sawFilterState = initLowPass( keyFrequency * 2, sampleRate, 0.2 );
        
    timbre = new Timbre( getSampleRate(),
                         1.0,
                         keyFrequency,
                         16, 
                         filteredSawWave,
                         1, 10 );

    envelope = new Envelope( 0.1, 0.8, 0.1,
                             1,
                             16,
                             gridStepSamples );
    
    }



void freeMusicPlayer() {
    delete timbre;
    delete envelope;
    }



int getLastColumnPlayed() {
    lockAudio();
    int val = lastPlayX;
    unlockAudio();
    
    return val;
    }



void setButtonPressed( int inX, int inY ) {
    lockAudio();
    pressedX = inX;
    pressedY = inY;
    
    unlockAudio();
    }




void hintBufferSize( int inLengthToFillInBytes ) {
    }



void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    
    int numSamples = inLengthToFillInBytes / 4;
    

    float *samplesL = new float[ numSamples ];
    float *samplesR = new float[ numSamples ];
    
    // first, zero-out the buffer to prepare it for our sum of note samples
    memset( samplesL, 0, sizeof( float ) * numSamples );
    memset( samplesR, 0, sizeof( float ) * numSamples );

    
    for( int i=0; i != numSamples; i++ ) {
        
        if( numSamplesPassed % gridStepSamples == 0 ) {
            // beginning of new step
            
            if( curNoteStepsleft == 0 ) {

                if( pressedX >= 0 && pressedY >= 0 ) {
                    
                    // start new note
                    curNotePitch = pressedY;
                    // 1 step, 2 steps, 4, steps, 8, or 16 steps
                    curNoteStepLength = pow( 2, pressedX );
                    curNoteStepsleft = curNoteStepLength - 1;
                    curNoteSamplesPassed = 0;
                    }
                else {
                    // rest
                    curNoteStepsleft = 0;
                    curNoteStepLength = 1;
                    curNoteSamplesPassed = 0;
                    curNotePitch = -1;
                    }
                }
            else {
                // don't play anything, step current note again
                curNoteStepsleft --;
                }
            }

        
        if( curNotePitch >=0 && curNoteStepLength > 0 &&
            curNotePitch < timbre->mNumWaveTableEntries ) {
            
            samplesL[i] = 
                envelope->getEnvelope( curNoteStepLength )
                    [ curNoteSamplesPassed ]
                *
                timbre->mWaveTable[ curNotePitch ]
                    [ curNoteSamplesPassed % 
                      timbre->mWaveTableLengths[ curNotePitch ] ];

            //printf( "%f samp\n", samplesL[i] );

            samplesR[i] = samplesL[i];
            }
        

        numSamplesPassed++;
        curNoteSamplesPassed++;
        }
    

    
    int streamPosition = 0;
    
    for( int i=0; i != numSamples; i++ ) {

        Sint16 intSampleL = (Sint16)( lrint( samplesL[i] ) );
        Sint16 intSampleR = (Sint16)( lrint( samplesR[i] ) );
        
        inBuffer[ streamPosition ] = (Uint8)( intSampleL & 0xFF );
        inBuffer[ streamPosition + 1 ] = (Uint8)( ( intSampleL >> 8 ) & 0xFF );
        
        inBuffer[ streamPosition + 2 ] = (Uint8)( intSampleR & 0xFF );
        inBuffer[ streamPosition + 3 ] = (Uint8)( ( intSampleR >> 8 ) & 0xFF );
        
        streamPosition += 4;
        }
    
    delete [] samplesL;
    delete [] samplesR;
    }
