#include "minorGems/game/game.h"
#include "minorGems/util/SimpleVector.h"

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


static Timbre *snareTimbre;
static Timbre *kickTimbre;

static Envelope *snareEnvelope;
static Envelope *kickEnvelope;


// c
//double keyFrequency = 261.63;

// actually, can't support high notes in the key of c w/out round-off errors
// because this is a wavetable implementation, and when the tables get short,
// the errors get huge
// THIS, however, evenly divides our sample rate (22050)
// which means that we get perfect, whole-number wave tables at octaves
static double keyFrequency = 172.265625 / 4;


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



typedef struct {
        Timbre *t;
        Envelope *e;
        
        int sampleDuration;
        int nextSample;
    } PlayingDrumNote;


SimpleVector<PlayingDrumNote> drumNotes;

    




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


// white noise where each sample is averaged with last sample
// effectively a low-pass filter
double lastNoiseSample = 0;

double smoothedWhiteNoise( double inT ) {
    // give double-weight to last sample to make it even smoother
    lastNoiseSample = ( 2 * lastNoiseSample + whiteNoise( inT ) ) / 3;
    
    return lastNoiseSample;
    }



// a sine wave that falls off over time
double sinThwip( double inT ) {
    return sin( inT / (1 + pow(inT, .2) ) ) ;
    }

double kickWave( double inT ) {
    return sinThwip( inT ) +
        // white noise at start, then fall-off
        smoothedWhiteNoise( inT ) * ( 1 - inT / (inT + 10 ) );
    }




#include "minorGems/sound/filters/coefficientFilters.h"


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
    /*
    double plainSaw = harmonicSaw( inT ) + harmonicSquare( inT ) + 
        whiteNoise( inT );
    */
    double plainSaw = sawWave( inT );
    
    double filteredSaw = coeffFilter( plainSaw, &sawFilterState );
    
    //if( tableCount < 2 ) {
    //    fprintf( sawFile, "%f %f %f\n", inT, plainSaw, filteredSaw );
    //    }

    return filteredSaw;
    }



CoeffFilterState globalFilterState;


void initMusicPlayer() {
    pressedX = -1;
    pressedY = -1;
    lastPlayX = -1;
    
    sampleRate = getSampleRate();
    
    initSumCoefficients();

    setDefaultScale();
    

    sawFilterState = initLowPass( keyFrequency*2, sampleRate, 0.21 );

    globalFilterState = initLowPass( keyFrequency*8, sampleRate, .5 );
        
    timbre = new Timbre( getSampleRate(),
                         .2,
                         keyFrequency,
                         16, 
                         //filteredSawWave,
                         harmonicSaw,
                         1, 10 );

    envelope = new Envelope( 0.00, 0.8, 0.0,
                             1,
                             16,
                             gridStepSamples );




     snareTimbre = new Timbre( sampleRate, 0.25,
                               keyFrequency/2,
                               1, smoothedWhiteNoise,
                               // extra periods per table so that
                               // noise doesn't become tonal through
                               // short looping
                               10 ); 

    snareEnvelope = new Envelope(
        0.0, 0.125, 0.0, 0.0,
        4,
        4,
        gridStepSamples );


    // kick drum type sound
    kickTimbre = new Timbre( sampleRate, 0.25,
                             keyFrequency,
                             1, kickWave,
                             // extra periods in table to make room
                             // for entire kick sweep
                             200 ); 

    kickEnvelope = new Envelope(
        // AHR model
        0.0, 0.25, 0.05,
        4,
        4,
        gridStepSamples );

    
    }



void freeMusicPlayer() {
    delete timbre;
    delete envelope;

    delete snareTimbre;
    delete kickTimbre;
    delete snareEnvelope;
    delete kickEnvelope;
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


void setGlobalFilterParams( double inCutoffFreq, double inRez ) {
    lockAudio();
    
    
    globalFilterState =  initLowPass( inCutoffFreq, sampleRate, inRez );
    
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
            
            int gridStepNumber = numSamplesPassed / gridStepSamples;
            
            if( false && gridStepNumber % 4 == 0 ) {
                PlayingDrumNote note = { snareTimbre, snareEnvelope,
                                         4 * gridStepSamples, 0 };
                drumNotes.push_back( note );
                }
            if( gridStepNumber % 4 == 0 ) {
                PlayingDrumNote note = { kickTimbre, kickEnvelope,
                                         4 * gridStepSamples, 0 };
                drumNotes.push_back( note );
                }
            
                

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
            
            samplesL[i] = coeffFilter( samplesL[i], &globalFilterState );

            samplesR[i] = samplesL[i];
            }
        
        for( int n=0; n<drumNotes.size(); n++ ) {
            PlayingDrumNote *note = drumNotes.getElement( n );
            
            samplesL[i] += note->e->getEnvelope( 4 )
                [ note->nextSample ]
                *
                note->t->mWaveTable[ 0 ]
                [ note->nextSample % 
                      note->t->mWaveTableLengths[ 0 ] ];
            samplesR[i] = samplesL[i];

            note->nextSample++;

            if( note->nextSample == note->sampleDuration ) {
                drumNotes.deleteElement( n );
                n--;
                }
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
