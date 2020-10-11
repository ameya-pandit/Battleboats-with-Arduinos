//HEAP SIZE: 1042 
//PARTNER HEAP SIZE: 1042

// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

//CMPE13 Support Library
#include "BOARD.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>

// User libraries
#include "Agent.h"
#include "CircularBuffer.h"
#include "Leds.h"
#include "Oled.h"
#include "Buttons.h"
#include "Protocol.h"
#include "Uart1.h"
#include "Field.h"
#include "OledDriver.h"
#include "FieldOled.h"

// **** Set any macros or preprocessor directives here ****

// **** Declare any data types here ****

// **** Define any module-level, global, or external variables here ****
static Field mySide;    //this is representing my playing field
static Field otherSide; //this is representing their playing field
static GuessData myGuess = {};  //this is representing my guess data - needed to hit/miss the boats
static GuessData otherGuess = {};   //this is representing their guess data - needed to hit/miss the boats
static NegotiationData myNegotiate = {};    //this is representing my negotiation data - needed to see who goes first and turn order
static NegotiationData otherNegotiate = {}; //this is representing their negotiation data - needed to see who goes first and turn order

static AgentState fsmState = AGENT_STATE_GENERATE_NEG_DATA; //setting the default FSM state to negotiate data (first state in state machine)
static AgentEvent agentEvent = AGENT_EVENT_NONE;    //setting the default agentEvent state to no event
static ProtocolParserStatus protoMessage = PROTOCOL_WAITING;    //setting the protocolMessage state to waiting - utilized to see what the agent event is doing
static TurnOrder turn = TURN_ORDER_TIE; //currently, the turn order is a tie - will be changed appropriately
FieldOledTurn currentTurn = FIELD_OLED_TURN_NONE;   //shows who turn it is

static int clockIncrement;  //this is utilized to generate the "wait" for the agent guess
static uint8_t check;   //this is utilized to determine and validate the turn data
static int storeRow;    //this is utilized in the guess state - if there is a hit, this value remembers where the row value of the hit was
static int storeCol;    //this is utilized in the guess state - if there is a hit, this value remembers where the column value of the hit was

// **** Declare any function prototypes here ****

/**
 * The Init() function for an Agent sets up everything necessary for an agent before the game
 * starts. This can include things like initialization of the field, placement of the boats,
 * etc. The agent can assume that stdlib's rand() function has been seeded properly in order to
 * use it safely within.
 */
void AgentInit(void){
    FieldInit(&mySide,FIELD_POSITION_EMPTY);    //set my side of the field
    FieldInit(&otherSide, FIELD_POSITION_UNKNOWN);  //set their side of the field
    //we do this to randomly place all boats - utilizing the concept of NOT doing something, find a situation where you can do it (the use of the !)
    while (!FieldAddBoat(&mySide, (rand() % FIELD_ROWS), (rand() % FIELD_COLS), 
        rand() % 4, FIELD_BOAT_SMALL)); //randomly places the small boat 
    while (!FieldAddBoat(&mySide, (rand() % FIELD_ROWS), (rand() % FIELD_COLS), 
        rand() % 4, FIELD_BOAT_MEDIUM));    //randomly places the medium boat
    while (!FieldAddBoat(&mySide, (rand() % FIELD_ROWS), (rand() % FIELD_COLS), 
        rand() % 4, FIELD_BOAT_LARGE)); //randomly places the large boat
    while (!FieldAddBoat(&mySide, (rand() % FIELD_ROWS), (rand() % FIELD_COLS), 
        rand() % 4, FIELD_BOAT_HUGE));  //randomly places the huge boat
    
    FieldOledDrawScreen(&mySide, &otherSide, currentTurn); //draw the screens
    OledUpdate();   //update
}

/**
 * The Run() function for an Agent takes in a single character. It then waits until enough
 * data is read that it can decode it as a full sentence via the Protocol interface. This data
 * is processed with any output returned via 'outBuffer', which is guaranteed to be 255
 * characters in length to allow for any valid NMEA0183 messages. The return value should be
 * the number of characters stored into 'outBuffer': so a 0 is both a perfectly valid output and
 * means a successful run.
 * @param in The next character in the incoming message stream.
 * @param outBuffer A string that should be transmit to the other agent. NULL if there is no
 *                  data.
 * @return The length of the string pointed to by outBuffer (excludes \0 character).
 */
int AgentRun(char in, char *outBuffer){ //this function implements the giant state machine that makes the ArtificialAgent work
    //We utilized this function to help notify the agent about receiving CHA and DET messages, amongst others - most useful and integral in generating the appropriate messages
    if (in != NULL) {   //if the message ISN'T NULL (valid)
        protoMessage = ProtocolDecode(in, &otherNegotiate, &otherGuess);    //DECODE the message
        
        if (protoMessage == PROTOCOL_PARSED_DET_MESSAGE)    {   //if the DET message received
            agentEvent = AGENT_EVENT_RECEIVED_DET_MESSAGE;      //notify agent of DET message receive
        } else if (protoMessage == PROTOCOL_PARSED_CHA_MESSAGE) {   //if the CHA message received
            agentEvent = AGENT_EVENT_RECEIVED_CHA_MESSAGE;      //notify agent of CHA message receive
        } else if (protoMessage == PROTOCOL_PARSED_COO_MESSAGE) {   //if the COO message received
            agentEvent = AGENT_EVENT_RECEIVED_COO_MESSAGE;      //notify agent of CHA message receive
        } else if (protoMessage == PROTOCOL_PARSED_HIT_MESSAGE) {   //if the HIT message received
            agentEvent = AGENT_EVENT_RECEIVED_HIT_MESSAGE;      //notify agent of HIT message receive
        } else if (protoMessage == PROTOCOL_PARSING_FAILURE)    {   //if there was a parsing failure
            agentEvent = AGENT_EVENT_MESSAGE_PARSING_FAILED;    //notify agent of a parsing failure - not good
        } else if (protoMessage == PROTOCOL_PARSING_GOOD)   {   //if the parsing was good, and if the protocol fsm is waiting, no event has been done/needs to be done
            agentEvent = AGENT_EVENT_NONE;
        } else if (protoMessage == PROTOCOL_WAITING)    {
            agentEvent = AGENT_EVENT_NONE;
        }
    }
    
    switch (fsmState)   {   //start of the giant state machine
        case AGENT_STATE_GENERATE_NEG_DATA: //generate appropriate negotiation data with the other kit
            ProtocolGenerateNegotiationData(&myNegotiate);  //sending in my data
            fsmState = AGENT_STATE_SEND_CHALLENGE_DATA; //switching to send challenge data
            
            int genChaMessage = 0;  //this is used to encode the CHA message needed to send
            genChaMessage = ProtocolEncodeChaMessage(outBuffer, &myNegotiate);
            
            return genChaMessage;   //return CHA message
            
            break;
            
        case AGENT_STATE_SEND_CHALLENGE_DATA:   //sending challenge data
            if (agentEvent == AGENT_EVENT_RECEIVED_CHA_MESSAGE) {   //if the appropriate CHA message from before received
                fsmState = AGENT_STATE_DETERMINE_TURN_ORDER;        //switching to determine turn order
                
                int genDetMessage = 0;  //this is used to encode the DET message needed to send
                genDetMessage = ProtocolEncodeDetMessage(outBuffer, &myNegotiate);
                
                return genDetMessage;   //return DET message
            } else if (agentEvent == AGENT_EVENT_MESSAGE_PARSING_FAILED)    {   //if there is a parsing failure
                OledDrawString(AGENT_ERROR_STRING_PARSING); //update the OLED, display the error message
                OledUpdate();
                fsmState = AGENT_STATE_INVALID; //set fsm to invalid
            } else if (agentEvent == AGENT_EVENT_NONE)  {   //if no event has occurred, do nothing (do not want to randomly generate an event)
                return 0;
            }
            break;
        
        case AGENT_STATE_DETERMINE_TURN_ORDER:  //case to determine the appropriate turn order
            if (agentEvent == AGENT_EVENT_RECEIVED_DET_MESSAGE) {   //if proper DET message received
                check = ProtocolValidateNegotiationData(&otherNegotiate);   //validate this data
                turn = ProtocolGetTurnOrder(&myNegotiate, &otherNegotiate); //determine the turn based on the turn order function in protocol
                
                if (check) {                        //if the negotiation data is valid
                    if (turn == TURN_ORDER_START)   {   //if it is my turn
                        currentTurn = FIELD_OLED_TURN_MINE; //set turn to mine
                        fsmState = AGENT_STATE_SEND_GUESS;  //switch fsm to send guess case
                        
                        FieldOledDrawScreen(&mySide, &otherSide, currentTurn);  //draw the screen the players/agent are playing on
                    } else if (turn == TURN_ORDER_TIE)  {   //if the turn determining sequence gave us a tie
                        fsmState = AGENT_STATE_INVALID; //this is invalid
             
                        OledDrawString(AGENT_ERROR_STRING_ORDERING);    //update display with error message about error ordering the playerse
                        OledUpdate();   //update
                    } else if (turn == TURN_ORDER_DEFER)    {   //if it is their turn
                        currentTurn = FIELD_OLED_TURN_THEIRS;   //set turn to their player
                        fsmState = AGENT_STATE_WAIT_FOR_GUESS;  //switch fsm to wait for guess case
                        
                        FieldOledDrawScreen(&mySide, &otherSide, currentTurn);  //draw the screen the players/agent are playing on
                    }
                } else {    //if the negotiation data is not valid
                    fsmState = AGENT_STATE_INVALID; //switch fsm to invalid
                    
                    OledDrawString(AGENT_ERROR_STRING_NEG_DATA);    //update OLED with error message that states there was an error negotiating the data
                    OledUpdate();   //update
                }
            } else if (agentEvent == AGENT_EVENT_MESSAGE_PARSING_FAILED)    {   //if there was an error parsing the message
                OledDrawString(AGENT_ERROR_STRING_PARSING); //display error message
                OledUpdate();   //update
                
                fsmState = AGENT_STATE_INVALID; //switch fsm to invalid
            } else if (agentEvent == AGENT_EVENT_NONE)  {   //if no event occurred, make sure nothing happens to the agent 
                return 0;
            }
            break;
        
        case AGENT_STATE_WAIT_FOR_GUESS:    //case for the loser of the negotiation data/turn order - we are waiting for the other players guess
            FieldOledDrawScreen(&mySide, &otherSide, currentTurn);  //draw screen
            
            if (AgentGetStatus() && agentEvent == AGENT_EVENT_RECEIVED_COO_MESSAGE) {   //if you received a coordinate message from the other player
                FieldRegisterEnemyAttack(&mySide, &otherGuess); //register the data
                
                currentTurn = FIELD_OLED_TURN_MINE; //it is now my turn
                fsmState = AGENT_STATE_SEND_GUESS;  //and I am now going to send my guess
                
                FieldOledDrawScreen(&mySide, &otherSide, currentTurn); //update the screen after their turn
                
                int genHitMessage = 0;  //utilized to generate the hit message
                genHitMessage = ProtocolEncodeHitMessage(outBuffer, &otherGuess);
                
                return genHitMessage;   //return the hit message
            } else if (agentEvent == AGENT_EVENT_MESSAGE_PARSING_FAILED)    {   //if there was an error parsing the message
                OledDrawString(AGENT_ERROR_STRING_PARSING); //display error message
                OledUpdate();   //update
                
                fsmState = AGENT_STATE_INVALID; //switch fsm to invalid
                return 0;
            } else if (AgentGetStatus() == 0)   {   //if they have no boats left
                currentTurn = FIELD_OLED_TURN_MINE; //it is now my turn
                fsmState = AGENT_STATE_LOST;    //and the other player has lost
                
                FieldOledDrawScreen(&mySide, &otherSide, currentTurn);  //update the screen
                
                int genHitMessageTwo = 0;   //generate another hit message
                genHitMessageTwo = ProtocolEncodeHitMessage(outBuffer, &otherGuess);
                
                return genHitMessageTwo;    //return this message
            } else if (agentEvent == AGENT_EVENT_NONE)  {   //if no event has occurred, the agent does nothing
                return 0;
            }
            break;
            
        case AGENT_STATE_SEND_GUESS:    //this is the case for the fsm to guess where to place their guess
            for (clockIncrement = 0; clockIncrement < (BOARD_GetPBClock() / 8); clockIncrement++);  //this is used for the delay as per the lab manual
                      
            while(1)    {   //this is always running in this case - continues to cycle through the cases and picks an "educated random guess"
                int checkHitValue = 0;  //check the value of a previous hit
                int flagHit = 0;    //this is a flag that keeps track of whether or not a previous guess was a hit
                
                checkHitValue = FieldAt(&otherSide, myGuess.row, myGuess.col);  //this gets the value using FieldAt of the opponents field with my guesses for row and column
                
                if (checkHitValue == FIELD_POSITION_HIT) {  //if my guesses resulted in a hit
                    storeRow = myGuess.row; //store the value of the row
                    storeRow = myGuess.col; //store the value of the column
                    flagHit = 1;    //set the flag to 1 - meaning there was a hit
                } else if (checkHitValue == FIELD_POSITION_UNKNOWN) {   //if the guesses do not give you a hit
                    break;  //break out of this loop as the guess did not result in a hit
                }
                
                if (flagHit == 1)   {   //if there was a hit value registered by the FieldAt function
                    int flagWest = 0;   //initializing these flag values that will randomly be involved in the "educated guess" portion of the send guess - as per the lab manual
                    int flagEast = 0;
                    int flagNorth = 0;
                    int flagSouth = 0;
                    
                    while(1)    {       
                        BoatDirection guessDirection = (rand() % 4);    //from the BoatDirection enum - you can go four different ways (0, 1, 2, 3)
                        
                        if (guessDirection == FIELD_BOAT_DIRECTION_WEST)    {   //if the random guess yields WEST
                            int checkWestValue = 0;  //this is used to check the FieldAt at a value West of the previous guess
                            checkWestValue = FieldAt(&otherSide, storeRow - 1, storeCol);
                            
                            if (checkWestValue == FIELD_POSITION_UNKNOWN)   {   //if the value yields an unknown value - a value which we haven't seen yet
                                myGuess.row = storeRow - 1; //the guess is now gone one unit to the left (West)
                                myGuess.col = storeCol;
                                break;
                            }
                            flagWest = 1;   //an event has occurred in this direction
                        } else if (guessDirection == FIELD_BOAT_DIRECTION_EAST) {   //if the random guess yields EAST
                            int checkEastValue = 0; //this is used to check the FieldAt at a value East of the previous guess
                            checkEastValue = FieldAt(&otherSide, storeRow + 1, storeCol);
                            
                            if (checkEastValue == FIELD_POSITION_UNKNOWN)   {   //if the value yields an unknown value - a value which we haven't seen yet
                                myGuess.row = storeRow + 1; //the guess is now gone one unit to the right (East)
                                myGuess.col = storeCol;
                                break;
                            }
                            flagEast = 1;   //an event has occurred in this direction
                        } else if (guessDirection == FIELD_BOAT_DIRECTION_NORTH)    {   //if the random guess yields NORTH
                            int checkNorthValue = 0;    //this is used to check the FieldAt a value North of the previous guess
                            checkNorthValue = FieldAt(&otherSide, storeRow, storeCol + 1);
                            
                            if (checkNorthValue == FIELD_POSITION_UNKNOWN)   {  //if the value yields an unknown value - a value which we haven't seen yet
                                myGuess.row = storeRow;
                                myGuess.col = storeCol + 1; //the guess is now one unit up (North)
                                break;
                            }
                            flagNorth = 1;  //an event has occurred in this direction
                        } else if (guessDirection == FIELD_BOAT_DIRECTION_SOUTH)    {   //if the random guess yields SOUTH
                            int checkSouthValue = 0;    //this is used to check the FieldAt a value South of the previoous guess
                            checkSouthValue = FieldAt(&otherSide, storeRow, storeCol - 1);
                            
                            if (checkSouthValue == FIELD_POSITION_UNKNOWN)   {  //if the value yields an unknown value - a value which we haven't seen yet
                                myGuess.row = storeRow;
                                myGuess.col = storeCol - 1; //the guess is now one unit down (South)
                                break;
                            }
                            flagSouth = 1;  //an event has occurred in this direction
                        }
                        if ((flagWest = 1) || (flagEast = 1) || (flagNorth = 1) || (flagSouth = 1)) {   //if an event in any direction has occurred
                            flagHit = 0;    //resetting the hit flag to zero - so that the cycle keeps going
                            break;
                        }
                    }
                }
                if (flagHit == 0)   {   //if the flagHit from before yields zero - meaning that the checkHitValue did not result in a hit - do a random guess
                    myGuess.row = rand() % 6;   //random row guess 
                    myGuess.col = rand() % 10;  //random column guess
                }
            }
            
            fsmState = AGENT_STATE_WAIT_FOR_HIT;    //switch fsm to WAIT_FOR_HIT after sending the guess
            
            int genCooMessage = 0;  //utilized to generate coo message
            genCooMessage = ProtocolEncodeCooMessage(outBuffer, &myGuess);
            
            return genCooMessage;   //return coo message
            
            break;
        
        case AGENT_STATE_WAIT_FOR_HIT:  //case for the wait for hit state
            if ((AgentGetEnemyStatus()) && (agentEvent == AGENT_EVENT_RECEIVED_HIT_MESSAGE))    {   //if the enemy still has boats and if they received a hit message
                currentTurn = FIELD_OLED_TURN_THEIRS;   //it is their turn
                fsmState = AGENT_STATE_WAIT_FOR_GUESS;  //fsm set to wait for guess
               
                FieldUpdateKnowledge(&otherSide, &otherGuess);  //update the knowledge of my field and their field
                FieldOledDrawScreen(&mySide, &otherSide, currentTurn);  //update the screen
                
                return 0;
            } else if (agentEvent == AGENT_EVENT_MESSAGE_PARSING_FAILED)    {   //if there was a parsing failure
                OledDrawString(AGENT_ERROR_STRING_PARSING); //display the parsing error message
                OledUpdate();   //update
                
                fsmState = AGENT_STATE_INVALID; //switch fsm to invalid
                return 0;
            } else if (AgentGetEnemyStatus() == 0)  {   //if they have no more boats left
                currentTurn = FIELD_OLED_TURN_NONE; //it is no ones turn
                fsmState = AGENT_STATE_WON; //they won
                
                FieldUpdateKnowledge(&otherSide, &otherGuess);  //update the screens
                FieldOledDrawScreen(&mySide, &otherSide, currentTurn); 
                
                return 0;
            } else if (agentEvent == AGENT_EVENT_NONE)  {   //if no event has occurred, the agentEvent is set to nothing
                return 0;
            }
            break;
        
        case AGENT_STATE_WON:   //if you win - as per the lab manual, this returns 0
            return 0;
            break;
        
        case AGENT_STATE_LOST:  //if you lose - as per the lab manual, this returns 0
            return 0;
            break;
        
        case AGENT_STATE_INVALID:   //if the state is invalid - as per the lab manual, this returns 0
            return 0;
            break;
    }
    return 0;
}

/**
 * StateCheck() returns a 4-bit number indicating the status of that agent's ships. The smallest
 * ship, the 3-length one, is indicated by the 0th bit, the medium-length ship (4 tiles) is the
 * 1st bit, etc. until the 3rd bit is the biggest (6-tile) ship. This function is used within
 * main() to update the LEDs displaying each agents' ship status. This function is similar to
 * Field::FieldGetBoatStates().
 * @return A bitfield indicating the sunk/unsunk status of each ship under this agent's control.
 *
 * @see Field.h:FieldGetBoatStates()
 * @see Field.h:BoatStatus
 */
uint8_t AgentGetStatus(void){
    return FieldGetBoatStates(&mySide); //return the state of our boats - how many lives left basically
}

/**
 * This function returns the same data as `AgentCheckState()`, but for the enemy agent.
 * @return A bitfield indicating the sunk/unsunk status of each ship under the enemy agent's
 *         control.
 *
 * @see Field.h:FieldGetBoatStates()
 * @see Field.h:BoatStatus
 */
uint8_t AgentGetEnemyStatus(void){
    return FieldGetBoatStates(&otherSide);  //return the state of their boats - how many lives they have left basically
}

