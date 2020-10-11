//HEAP SIZE: 1042
//PARTNER HEAP SIZE: 1042

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Protocol.h"
#include "Field.h"
#include "FieldOled.h"

typedef enum {  //this is my enum for all the states, as asked for in the lab manual
    WAITING,
    RECORDING, 
    FIRST_CHECKSUM_HALF,
    SECOND_CHECKSUM_HALF,
    NEWLINE
} protocolState;

typedef struct ProtocolData {   //this is my struct for all the data, as asked for in the lab manual
    char protocolMessage[PROTOCOL_MAX_MESSAGE_LEN+1];   //Professor Dunne enlarged our message length (array) by 1 as a test
    int protocolIndex;  //this is the index we use to run through the message
    uint8_t checksumVal;    //this is the value of the checksum of the message
    protocolState state;    //this is the state of the aforementioned enum
} ProtocolData;

static ProtocolData strData;    

static char *newlineData;   //this stores the tokenized string - utilized to distinguish between the four types of messages (DET, CHA, COO, HIT)
static char *dataOne;       //this is the first tokenized value
static char *dataTwo;       //this is the second tokenized value
static char *dataThree;     //this is the third tokenized value for the big messages

static uint8_t passedHexValue;  //this is a variable utilized to measure of the passed Hex char is valid)
static uint8_t checkChecksumValue;  //this is the variable used to compare the checksum values 

static uint8_t Checksum(char *dataString);  //this is the checksum function - it calculates the checksum of the string
static uint8_t AsciiConverter(char asciiValue); //this is the ASCII converter - vital for hex conversions
static void ClearString(char *clrMessage);  //this function isn't super necessary but as one of the TAs recommended it I added it to make it easier to clear the string

/**
 * Encodes the coordinate data for a guess into the string `message`. This string must be big
 * enough to contain all of the necessary data. The format is specified in PAYLOAD_TEMPLATE_COO,
 * which is then wrapped within the message as defined by MESSAGE_TEMPLATE. The final length of this
 * message is then returned. There is no failure mode for this function as there is no checking
 * for NULL pointers.
 * @param message The character array used for storing the output. Must be long enough to store the
 *                entire string, see PROTOCOL_MAX_MESSAGE_LEN.
 * @param data The data struct that holds the data to be encoded into `message`.
 * @return The length of the string stored into `message`.
 */
int ProtocolEncodeCooMessage(char *message, const GuessData *data)  {   //this function encodes a coordinate message
    if (message != NULL){   //if the pointer points to a message that isn't NULL 
        ClearString(message);   //clear the message
    }
    
    char encodeCooMessage[PROTOCOL_MAX_PAYLOAD_LEN] = {};   //initialize the coo array
    
    sprintf(encodeCooMessage, PAYLOAD_TEMPLATE_COO, data->row, data->col);  //this follows the format presented in Protocol.h
    sprintf(message, MESSAGE_TEMPLATE, encodeCooMessage, Checksum(encodeCooMessage));   //this follows the same principles
    
    //I tested this by calling this function in the tester file and passing in a string, checking if the output I expected was the same
    
    return strlen(message);
}

/**
 * Follows from ProtocolEncodeCooMessage above.
 */
int ProtocolEncodeHitMessage(char *message, const GuessData *data)  {   //this function encodes a hit message
    if (message != NULL){   //if the pointer points to a message that isn't NULL
        ClearString(message);   //clear the message
    }
    
    char encodeHitMessage[PROTOCOL_MAX_PAYLOAD_LEN] = {};   //initialize the hit array
    
    sprintf(encodeHitMessage, PAYLOAD_TEMPLATE_HIT, data->row, data->col, data->hit);   //this follows the format presented in Protocol.h
    sprintf(message, MESSAGE_TEMPLATE, encodeHitMessage, Checksum(encodeHitMessage));   //this follows the same principles
    
    //I tested this by calling this function in the tester file and passing in a string, checking if the output I expected was the same

    return strlen(message);
}

/**
 * Follows from ProtocolEncodeCooMessage above.
 */
int ProtocolEncodeChaMessage(char *message, const NegotiationData *data)    {   //this functions encodes a cha message
    if (message != NULL){   //if the pointer points to a message that isn't NULL 
        ClearString(message);   //clear the message
    }    
    
    char encodeChaMessage[PROTOCOL_MAX_PAYLOAD_LEN] = {};   //initialize the cha array
    
    sprintf(encodeChaMessage, PAYLOAD_TEMPLATE_CHA, data->encryptedGuess, data->hash);  //this follows the format presented in Protocol.h
    sprintf(message, MESSAGE_TEMPLATE, encodeChaMessage, Checksum(encodeChaMessage));   //this follows the same principles
    
    //I tested this by calling this function in the tester file and passing in a string, checking if the output I expected was the same

    return strlen(message);
}

/**
 * Follows from ProtocolEncodeCooMessage above.
 */
int ProtocolEncodeDetMessage(char *message, const NegotiationData *data)    {   //this function encodes a det message
    if (message != NULL){   //if the pointer points to a message that isn't NULL
        ClearString(message);   //clear the message
    }
    
    char encodeDetMessage[PROTOCOL_MAX_PAYLOAD_LEN] = {};   //initializes the det array
    
    sprintf(encodeDetMessage, PAYLOAD_TEMPLATE_DET, data->guess, data->encryptionKey);  //this follows the format presented in Protocol.h
    sprintf(message, MESSAGE_TEMPLATE, encodeDetMessage, Checksum(encodeDetMessage));   //this follows the same principles
    
    //I tested this by calling this function in the tester file and passing in a string, checking if the output I expected was the same

    return strlen(message);
}

/**
 * This function decodes a message into either the NegotiationData or GuessData structs depending
 * on what the type of message is. This function receives the message one byte at a time, where the
 * messages are in the format defined by MESSAGE_TEMPLATE, with payloads of the format defined by
 * the PAYLOAD_TEMPLATE_* macros. It returns the type of message that was decoded and also places
 * the decoded data into either the `nData` or `gData` structs depending on what the message held.
 * The onus is on the calling function to make sure the appropriate structs are available (blame the
 * lack of function overloading in C for this ugliness).
 *
 * PROTOCOL_PARSING_FAILURE is returned if there was an error of any kind (though this excludes
 * checking for NULL pointers), while
 * 
 * @param in The next character in the NMEA0183 message to be decoded.
 * @param nData A struct used for storing data if a message is decoded that stores NegotiationData.
 * @param gData A struct used for storing data if a message is decoded that stores GuessData.
 * @return A value from the UnpackageDataEnum enum.
 */
ProtocolParserStatus ProtocolDecode(char in, NegotiationData *nData, GuessData *gData)  {   //this function is the function responsible for DECODING the messages
    switch (strData.state)  {   //I made a switch as this works like a FSM
        case WAITING:   //in the waiting case
            if (in == '$')   {  //if the input is valid
                ClearString(strData.protocolMessage);   //CLEAR THE STRING JUST IN CASE as not doing so might result in a freeze (this happened to us)
                strData.protocolIndex = 0;  //initializing the value of the index
                strData.state = RECORDING;  //switching states
                return PROTOCOL_PARSING_GOOD;   
            } else {
               return PROTOCOL_WAITING; 
            }
            break;
        case RECORDING: //if the recording case
            if (in == '*')  {   //if the input is at the end
                strData.state = FIRST_CHECKSUM_HALF;    //switch states
                return PROTOCOL_PARSING_GOOD; 
            } else if (in != '*')   {   //if the input isn't at the end of the string
                strData.protocolMessage[strData.protocolIndex] = in;    //increment through the string till so
                strData.protocolIndex++;
               return PROTOCOL_PARSING_GOOD;
            }
            break;
        case FIRST_CHECKSUM_HALF:   //in the first checksum half case
            passedHexValue = AsciiConverter(in);    //utilized to check if valid hex character
            
            if (passedHexValue < 16)    {   //if valid hex character
                strData.checksumVal = passedHexValue << 4;  //saving top four bits
                strData.state = SECOND_CHECKSUM_HALF;   //switch states
                return PROTOCOL_PARSING_GOOD;
            } else if (passedHexValue > 15) {   //if invalid hex character
                strData.state = WAITING;    //go back to waiting
                return PROTOCOL_PARSING_FAILURE;
            }
            break;
        case SECOND_CHECKSUM_HALF:  //in the second checksum half case
            passedHexValue = AsciiConverter(in);    //utilized to check if valid hex character
            checkChecksumValue = Checksum(strData.protocolMessage); //utilized to compare checksums
            
            strData.checksumVal |= passedHexValue;  //storing - bitwise or
        
            if (passedHexValue < 16 && checkChecksumValue == strData.checksumVal)   {   //if valid hex character and checksums match
                strData.protocolMessage[strData.protocolIndex] = '\0';  //clear string off
                strData.state = NEWLINE;    //switch states
                return PROTOCOL_PARSING_GOOD;
            } else if (passedHexValue > 15 || strData.checksumVal != checkChecksumValue){   //invalid case
                strData.state = WAITING;    //go to waiting
                return PROTOCOL_PARSING_FAILURE;
            }
            break;
        case NEWLINE:   //in the newline case
            strData.state = WAITING;    //new string - go to waiting
            int checkValidData = 0;     //utilized to keep track of valid data count
            
            if (in == '\n') {   //if the string is gone through
                newlineData = strtok(strData.protocolMessage, ","); //tokenize the string
          
                if ((strncmp(strData.protocolMessage, "DET", 3)) == 0)  {   //if the tokenized string gives you a DET case
                    dataOne = strtok(NULL, ",");    //tokenize this DET case even more, getting the guess
                    dataTwo = strtok(NULL, "*");    //getting the encryptionKey
                    nData->guess = atoi(dataOne);   //store the guess
                    nData->encryptionKey = atoi(dataTwo);   //store the key
                    checkValidData = 1; //this was valid data
                   
                    return PROTOCOL_PARSED_DET_MESSAGE;
                } else if ((strncmp(strData.protocolMessage, "CHA", 3)) == 0)   {   //if the tokenized string gives you a CHA case
                    dataOne = strtok(NULL, ",");    //tokenize this CHA case even more, getting the encrypted guess
                    dataTwo = strtok(NULL, "*");    //getting the hash
                    nData->encryptedGuess = atoi(dataOne);  //store the encrypted guess
                    nData->hash = atoi(dataTwo);    //store the hash
                    checkValidData = 1; //this was valid data

                    return PROTOCOL_PARSED_CHA_MESSAGE;
                } else if ((strncmp(strData.protocolMessage, "COO", 3)) == 0)   {   //if the tokenized string gives you a COO case
                    dataOne = strtok(NULL, ",");    //tokenize this COO case even more, getting the row
                    dataTwo = strtok(NULL, "*");    //getting the column
                    gData->row = AsciiConverter(*dataOne);  //storing the row
                    gData->col = AsciiConverter(*dataTwo);  //storing the column
                    checkValidData = 1; //this was valid data

                    return PROTOCOL_PARSED_COO_MESSAGE;
                } else if ((strncmp(strData.protocolMessage, "HIT", 3)) == 0)   {   //if the tokenized string gives you a HIT case
                    dataOne = strtok(NULL, ",");    //tokenize this HIT case even more, getting the row
                    dataTwo = strtok(NULL, ",");    //getting the column
                    dataThree = strtok(NULL, "*");  //getting the hit
                    gData->row = AsciiConverter(*dataOne);  //storing the row
                    gData->col = AsciiConverter(*dataTwo);  //storing the column
                    gData->hit = AsciiConverter(*dataThree);    //storing the hit
                    checkValidData = 1; //this was valid data
           
                    return PROTOCOL_PARSED_HIT_MESSAGE;
                }
            } else if (checkValidData == 0 || in != '\n') { //if the newline wasn't reached or the data was not valid, this was a fail
                return PROTOCOL_PARSING_FAILURE;
            }
            break;
    }
    
    return PROTOCOL_PARSING_FAILURE;
}

/**
 * This function generates all of the data necessary for the negotiation process used to determine
 * the player that goes first. It relies on the pseudo-random functionality built into the standard
 * library. The output is stored in the passed NegotiationData struct. The negotiation data is
 * generated by creating two random 16-bit numbers, one for the actual guess and another for an
 * encryptionKey used for encrypting the data. The 'encryptedGuess' is generated with an
 * XOR(guess, encryptionKey). The hash is simply an 8-bit value that is the XOR() of all of the
 * bytes making up both the guess and the encryptionKey. There is no checking for NULL pointers
 * within this function.
 * @param data The struct used for both input and output of negotiation data.
 */ 
void ProtocolGenerateNegotiationData(NegotiationData *data) {   //generate the data to determine who goes first
    uint16_t encryptionKey = (rand() & 0xFFFF); //use rand() for random numbers, used mask to mask off the excess bits
    uint16_t guess = (rand() & 0xFFFF); 
    
    data->encryptionKey = encryptionKey;    //storing the random key
    data->guess = guess;    //storing the random guess
    
    data->encryptedGuess = (data->guess ^ data->encryptionKey); //encrypting the guess with the key
    
    uint8_t hash = (guess >> 8) & 0xFF; //the hash XORs the bits from the guess and the key (first and second half)
    hash ^= guess & 0xFF;   
    hash ^= (encryptionKey >> 8) & 0xFF;
    hash ^= encryptionKey & 0xFF;
    
    data->hash = hash;  //storing the hash
}

/**
 * Validates that the negotiation data within 'data' is correct according to the algorithm given in
 * GenerateNegotitateData(). Used for verifying another agent's supplied negotiation data. There is
 * no checking for NULL pointers within this function. Returns TRUE if the NegotiationData struct
 * is valid or FALSE on failure.
 * @param data A filled NegotiationData struct that will be validated.
 * @return TRUE if the NegotiationData struct is consistent and FALSE otherwise.
 */
uint8_t ProtocolValidateNegotiationData(const NegotiationData *data)    {   //validate the data to determine who goes first - work backwards from Generate function
    uint16_t checkGuess = (data->guess ^ data->encryptionKey);  //utilized to decrypt the guess
    
    uint8_t checkHash = (data->guess >> 8) & 0xFF;  //utilized to decrypt the hash
    checkHash ^= (data->guess) & 0xFF;
    checkHash ^= (data->encryptionKey >> 8) & 0xFF;
    checkHash ^= (data->encryptionKey) & 0xFF;
   
    if ((checkGuess == data->encryptedGuess) && (checkHash == data->hash))   {  //if the guesses match and the hashes match, return success
        return 1;
    } else {
        return 0;
    }
}

/**
 * This function returns a TurnOrder enum type representing which agent has won precedence for going
 * first. The value returned relates to the agent whose data is in the 'myData' variable. The turn
 * ordering algorithm relies on the XOR() of the 'encryptionKey' used by both agents. The least-
 * significant bit of XOR(myData.encryptionKey, oppData.encryptionKey) is checked so that if it's a
 * 1 the player with the largest 'guess' goes first otherwise if it's a 0, the agent with the
 * smallest 'guess' goes first. The return value of TURN_ORDER_START indicates that 'myData' won,
 * TURN_ORDER_DEFER indicates that 'oppData' won, otherwise a tie is indicated with TURN_ORDER_TIE.
 * There is no checking for NULL pointers within this function.
 * @param myData The negotiation data representing the current agent.
 * @param oppData The negotiation data representing the opposing agent.
 * @return A value from the TurnOrdering enum representing which agent should go first.
 */
TurnOrder ProtocolGetTurnOrder(const NegotiationData *myData, const NegotiationData *oppData)   {   //this function determines the turn order between the two kits
    if (((myData->encryptionKey ^ oppData->encryptionKey) & 0x01) == 1) {   //getting least sig bit - one
        if (myData->guess > oppData->guess) {   //if my guess was bigger than theirs, I start
            return TURN_ORDER_START;
        } else if (myData->guess < oppData->guess) {    //if their guess was bigger than mine, they start
            return TURN_ORDER_DEFER;
        }
    } else  {   //the value was zero
        if (myData->guess < oppData->guess) {   //if their guess is bigger, they go first
            return TURN_ORDER_START;
        } else if (myData->guess > oppData->guess) {    //if my guess is bigger, I go first
            return TURN_ORDER_DEFER;
        }
    }
    return TURN_ORDER_TIE;  //otherwise, this is a tie
}

static uint8_t Checksum(char *dataString) { //this is the checksum function
    uint8_t checksumValue = 0;
    int incChecksum = 0;
    
    while (incChecksum < strlen(dataString))    {   //this runs through the checksum string 
        checksumValue ^= dataString[incChecksum];   //XORing the bits
        incChecksum++;  //incrementing
    }
   
    return checksumValue;       //return the value of the checksum
}

static uint8_t AsciiConverter(char asciiValue)  {   //this is my ASCII converter
    uint8_t hexValue;
    
    if (asciiValue >= '0' && asciiValue <= '9') {   //the numbers
        hexValue = (uint8_t)asciiValue - 48;
    } else if (asciiValue >= 'a' && asciiValue <= 'f')  {   //the smaller case letters
        hexValue = (uint8_t)asciiValue - 87;
    } else if (asciiValue >= 'A' && asciiValue <= 'F')   {  //the bigger case letters
        hexValue = (uint8_t)asciiValue - 55;
    }
    return hexValue;    //return the HEX value
}

static void ClearString(char *clrMessage)   {   //this function CLEARS the string
    int clrIncrement;
    for (clrIncrement = 0; clrIncrement < PROTOCOL_MAX_MESSAGE_LEN; clrIncrement++) {
        *clrMessage = '\0'; //set the pointer to NULL
        clrMessage++;   //increment
    }
}