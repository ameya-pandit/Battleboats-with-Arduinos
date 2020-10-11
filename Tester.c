// **** Include libraries here ****
// Standard libraries

//CMPE13 Support Library
#include "BOARD.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>

// User libraries
#include "Buttons.h"
#include "Oled.h"
#include "Protocol.h"
#include "Field.h"
#include "FieldOled.h"

// **** Set any macros or preprocessor directives here ****

// **** Declare any data types here ****

// **** Define any module-level, global, or external variables here ****
static uint32_t counter;
static uint8_t buttonEvents;

// **** Declare any function prototypes here ****



int main()
{
    BOARD_Init();

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a 10ms timer.
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, BOARD_GetPBClock() / 16 / 100);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T2);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T2, INT_ENABLED);

    // Disable buffering on stdout
    setbuf(stdout, NULL);

    ButtonsInit();

    OledInit();

    // Prompt the user to start the game and block until the first character press.
    OledDrawString("Press BTN4 to start.");
    OledUpdate();
    while ((buttonEvents & BUTTON_EVENT_4UP) == 0);
        // The first part of our seed is a hash of the compilation time string. The lowest-8 bits
    // are xor'd from the first-half of the string and the highest 8-bits are xor'd from the
    // second-half of the string.
    char seed1[] = __TIME__;
    int seed1_len = strlen(seed1);
    int firstHalf = seed1_len / 2;
    uint16_t seed2 = 0;
    int i;
    for (i = 0; i < seed1_len; ++i) {
        seed2 ^= seed1[i] << ((i < firstHalf) ? 0 : 8);
    }

    // Now we hash-in the time since first user input (which, as a 32-bit number, is split
    // and each half is hashed in.
    srand(seed2 ^ ((uint16_t) (counter >> 16)) ^ (uint16_t) (counter));


/******************************************************************************
 * Your code goes in between this comment and the following one with asterisks.
 *****************************************************************************/
    //test supplemental functions
    //test main functions w/ data passed in
    //test decode 
            // takes a char one at a time with a loop and see if it reaches a problem
            //pass in message with an error to see if it can be caught

//    char Message[100];
//    GuessData myGuess;
//    myGuess.row = 5;
//    myGuess.col = 9;
//    myGuess.hit = 0;
//    ProtocolEncodeHitMessage(Message, &myGuess);
//    printf("%s\n", Message);
    
//    char Message2[100];
      NegotiationData negData;
//    negData.encryptedGuess = 42233;
//    negData.hash = 93;
//    ProtocolEncodeChaMessage(Message2, &negData);
//    printf("%s\n", Message2);
//    
//    char Message3[100];
//    negData.guess = 47822;
//    negData.encryptionKey = 7735;
//    ProtocolEncodeDetMessage(Message3, &negData);
//    printf("%s\n", Message3);
    
//    printf("\nNEW TEST\n");
    GuessData guessData;
//    ProtocolGenerateNegotiationData(&negData);
//    ProtocolValidateNegotiationData(&negData);
//    
    char check[] = "$COO,1,3*41\n";
    int inc;
    
    for(inc=0;inc<strlen(check);inc++)    {
        ProtocolDecode(check[inc], &negData, &guessData);
    }
    printf("\nrow: %d, col: %d\n", guessData.row, guessData.col);
//    printf("\nGuess: %d, eKey: %d\n", negData.guess, negData.encryptionKey);
    //printf("\neGuess: %d, hash: %d\n", negData.encryptedGuess, negData.hash);
    //printf("row: %d, col: %d, hit: %d", guessData.row, guessData.col, guessData.hit);
    
    char checkTwo[] = "$COO,1,3";
    int incTwo;
    
    for(incTwo = 0;strlen(checkTwo);incTwo++)   {
        Checksum(checkTwo[incTwo]);
    }
    
    printf("\n\n\n------TEST-----\n\n");
    
    /*Field enemyField;
    printf("Hello");
    FieldInit(&enemyField,FIELD_POSITION_EMPTY);
    printf("end of init");*/
    
//    Field myField;
//    FieldInit(&myField,FIELD_POSITION_EMPTY);
//    FieldAddBoat(&myField, 0, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_SMALL);
//    FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_MEDIUM);
//    FieldAddBoat(&myField, 1, 0, FIELD_BOAT_DIRECTION_EAST, FIELD_BOAT_HUGE);
//    FieldAddBoat(&myField, 0, 5, FIELD_BOAT_DIRECTION_SOUTH, FIELD_BOAT_HUGE);
//   
//    FieldPosition a = FieldAt(&myField, 0, 2);
//    FieldPosition b = FieldSetLocation(&myField, 0, 2, 0);
    
//    if (a != NULL)  {
//        printf("success\n");
//    } else  {
//        printf("doesn't exist\n");
//    }
//    
//    if (b != NULL)  {
//        printf("ayee success");
//    } else  {
//        printf("nah");
//    }
    
    

//    #define PrintField
//    #ifdef PrintField
//    int inc = 0;
//    int j = 0;
//    for (inc = 0; inc < FIELD_ROWS; inc++) {
//        for (j = 0; j < FIELD_COLS; j++) {
//            printf(" %d ", myField.field[inc][j]);
//        }
//        printf("\n");
//    }
//    #endif

    
/******************************************************************************
 * Your code goes in between this comment and the preceeding one with asterisks
 *****************************************************************************/

    while (1);
}



/**
 * This is the interrupt for the Timer2 peripheral. It just keeps incrementing a counter used to
 * track the time until the first user input.
 */
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    // Increment a counter to see the srand() function.
    counter++;

    // Also check for any button events
    buttonEvents = ButtonsCheckEvents();
}