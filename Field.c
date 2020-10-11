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

//Initialize FieldInit
void FieldInit(Field *f, FieldPosition p){
    int i;// = 0;
    int j;// = 0;
    
    f->hugeBoatLives = FIELD_BOAT_LIVES_HUGE; //total of 6 lives
    f->largeBoatLives = FIELD_BOAT_LIVES_LARGE; //total of 5 lives
    f->mediumBoatLives = FIELD_BOAT_LIVES_MEDIUM; //total of 4 lives
    f->smallBoatLives = FIELD_BOAT_LIVES_SMALL; //total of 3 lives
    
    for(i = 0; i < FIELD_ROWS; i++){
        //increment rows
        for(j = 0; j < FIELD_COLS; j++){
           //increment columns
            f->field[i][j] = p; //Current FieldPosition
        }
        
        //i++;
    }
}

//Initialize FieldAt
FieldPosition FieldAt(const Field *f, uint8_t row, uint8_t col){
        return f->field[row][col]; //returns wanted position in terms of row and columns
    //}
    //else{
        //return STANDARD_ERROR; //returns error
    //}
}

//Initialize FieldSetLocation
FieldPosition FieldSetLocation(Field *f, uint8_t row, uint8_t col, FieldPosition p){
    FieldPosition ogPos = f->field[row][col]; //the original position
    f->field[row][col] = p;
    return ogPos; //returns original position of boat
}

//Initialize FieldAddBoat
uint8_t FieldAddBoat(Field *f, uint8_t row, uint8_t col, BoatDirection dir, BoatType type){
    int boatLength = 0; //initialized boat
    int newRow = 0; //initialize rows
    int newCol = 0; //initialize cols
    FieldPosition p = FIELD_POSITION_EMPTY; //set p as field position empty
    if (((row + 1) > FIELD_ROWS) || ((col + 1) > FIELD_COLS)) {//if either rows or col increase
        return STANDARD_ERROR; //beyond the given field rows/cols, returns error
    }
    switch (type){
    case FIELD_BOAT_SMALL: //when boat is small
        boatLength = FIELD_BOAT_LIVES_SMALL; //adds length of small boat
        p = FIELD_POSITION_SMALL_BOAT;//position of small boat
        break;
    case FIELD_BOAT_MEDIUM: //when boat is medium
        boatLength = FIELD_BOAT_LIVES_MEDIUM;//adds length of medium boat
        p = FIELD_POSITION_MEDIUM_BOAT;//position of med boat
        break;
    case FIELD_BOAT_LARGE: //when boat is large
        boatLength = FIELD_BOAT_LIVES_LARGE;//adds length of large boat
        p = FIELD_POSITION_LARGE_BOAT;//position of large boat
        break;
    case FIELD_BOAT_HUGE://when boat is huge
        boatLength = FIELD_BOAT_LIVES_HUGE;//adds length of huge boat
        p = FIELD_POSITION_HUGE_BOAT;//position of huge boat
        break;
    }
    //allows boats to move in certain directions
    switch(dir){
    case FIELD_BOAT_DIRECTION_NORTH: //Northern direction
        if((1 + row) - boatLength >= 0){//
            newRow = row;
            newCol = boatLength;
            while(newCol > 0){//when newcol is greater than 0
                if (f->field[newRow][col] != FIELD_POSITION_EMPTY){
                    return STANDARD_ERROR;
                }
                else{ //will decrement rows if error doesnt occur
                    newRow--;
                }
                newCol--; //decrements col regardless
            }
            while(boatLength > 0){ //also when length of boat is greater than initial value
                f->field[row][col] = p;//sets coordinates
                row--; //decrements row
                boatLength--; //with length of boat
            }
        }
        else{
            return FALSE; //otherwise false
        }
        break;
    case FIELD_BOAT_DIRECTION_SOUTH: //southern direction 
        if ((1 + row) + boatLength <= (FIELD_ROWS + 1)) {//add 1 to fieldrow to adjust boats
            newRow = row;
            newCol = boatLength;
            while (newCol > 0) { //if new col is greater than initial val
                if (f->field[newRow][col] != FIELD_POSITION_EMPTY) {//if field of newrow and col
                    return STANDARD_ERROR; //isn't empty, returns error
                } else {
                    newRow++;//increment rows when not empty
                }
                newCol--; //decrement col when greater than 0
            }
            while (boatLength > 0) { //also when length of boat is greater than initial value
                f->field[row][col] = p;
                row++; //increment rows
                boatLength--; //with decreasing boat length
            }
        }
        else {
            return FALSE;//otherwise fails
        }
        break;
    case FIELD_BOAT_DIRECTION_EAST: //east direction
        if ((1 + col) + boatLength <= (FIELD_COLS + 1)) { //add 1 to field cols to adjust boats
            newCol = boatLength;
            newRow = col;
            while (newCol > 0) {
                if (f->field[row][newRow] != FIELD_POSITION_EMPTY) { //when not empty
                    return STANDARD_ERROR;
                } else {
                    newRow++; //increment newrow when empty
                }
                newCol--; //decrements col when newcol is greater than initial val
            }
            while (boatLength > 0) {
                f->field[row][col] = p;
                col++;
                boatLength--;
            }
        } 
        else {
            return FALSE; //otherwise fails
        }
        break;
    case FIELD_BOAT_DIRECTION_WEST: //west direction
        if ((1 + col) - boatLength >= 0) {
            newRow = col;
            newCol = boatLength;
            while (newCol > 0) { //if col is greater than 0
                if (f->field[row][newRow] != FIELD_POSITION_EMPTY) { //return standard error when
                    return STANDARD_ERROR; //not empty
                } 
                else {
                    newRow--;//decrement row
                }
                newCol--;//decrement newcol
            }
            while (boatLength > 0) { 
                f->field[row][col] = p;
                boatLength--;
                col--;
            }
        } 
        else {
            return FALSE;
        }
        break;
    }
    //else {
    //    return FALSE;
    //}
    return SUCCESS; //return success
}



//Initialize FieldRegisterEnemyAttack
FieldPosition FieldRegisterEnemyAttack(Field *f, GuessData *gData){
    int ogPos = f->field[gData->row][gData->col]; //initialize position of enemy attacks
    switch(ogPos){ //uses switch cases for determining when each type of boat is hit/sunk
        case FIELD_POSITION_SMALL_BOAT: //starts with small boat
        
        if (f->smallBoatLives != 0) { //when still has some lives
            gData->hit = HIT_HIT; //a hit will go through
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT; //marks hit
            f->smallBoatLives--; //decrements lives
            return FIELD_POSITION_SMALL_BOAT; //returns boat with updated lives
            
        } else if (f->smallBoatLives == 0){ //if lives are equal to 0 however,
            gData->hit = HIT_SUNK_SMALL_BOAT; //marks as sunk
            return FIELD_POSITION_SMALL_BOAT; //returns sunken boat
            
        }break;
        case FIELD_POSITION_MEDIUM_BOAT:
        
        if (f->mediumBoatLives != 0) {//when still has some lives
            gData->hit = HIT_HIT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT;//marks hit
            f->mediumBoatLives--; //decrements life of medium boat
            return FIELD_POSITION_MEDIUM_BOAT; //returns update life of boat
            
        }  else if (f->mediumBoatLives == 0){ //when lives are zero
            gData->hit = HIT_SUNK_MEDIUM_BOAT;
            return FIELD_POSITION_MEDIUM_BOAT; //returns sunk boat
            
        }
        break;
        case FIELD_POSITION_LARGE_BOAT:
       
        if (f->largeBoatLives != 0) { //still has some lives
            gData->hit = HIT_HIT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT; //counts hit
            f->largeBoatLives--; //decrements life
            return FIELD_POSITION_LARGE_BOAT; //returns updated life of boat
            
        }
        else if (f->largeBoatLives == 0){ //when lives are zero
            gData->hit = HIT_SUNK_LARGE_BOAT;
            return FIELD_POSITION_LARGE_BOAT; //returns sunk boat
        }
        break;
        case FIELD_POSITION_HUGE_BOAT:
       
        if (f->hugeBoatLives != 0) { //when lives are not zero
            gData->hit = HIT_HIT;
            f->field[gData->row][gData->col] = FIELD_POSITION_HIT;//counts hit
            f->hugeBoatLives--; //decrements remaining life
            return FIELD_POSITION_HUGE_BOAT; //returns updated lives of boat
            
        }
        else if (f->hugeBoatLives == 0){ //when lives are zero
            gData->hit = HIT_SUNK_HUGE_BOAT;
            return FIELD_POSITION_HUGE_BOAT; //returns sunk boat
        }
        break;
        case FIELD_POSITION_EMPTY: //if none of the above occur, marks as empty
        gData->hit = HIT_MISS; //counts as miss
        f->field[gData->row][gData->col] = FIELD_POSITION_MISS; //returns miss marker
        return FIELD_POSITION_EMPTY;//returns empty position
         break;
    }
   
    return (ogPos); //returns original position case went through 
    }


//Initialize FieldUpdateKnowledge
FieldPosition FieldUpdateKnowledge(Field *f, const GuessData *gData){
    int ogPos = 0;
    ogPos = f->field[gData->row][gData->col]; // using original position again
    
    if (gData->hit == HIT_HIT){ //When hit occurs
        f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
    } else if (gData->hit == HIT_MISS){
        f->field[gData->row][gData->col] = FIELD_POSITION_EMPTY;
    }
    if (gData->hit == HIT_SUNK_SMALL_BOAT){
        f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
        f->smallBoatLives = 0; //No more lives on small boat
    }
    else if (gData->hit == HIT_SUNK_MEDIUM_BOAT){
        f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
        f->mediumBoatLives = 0; //no more lives on medium boat
    }
    else if (gData->hit == HIT_SUNK_LARGE_BOAT){
        f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
        f->largeBoatLives = 0; //no more lives on large boat
    }
    else if (gData->hit == HIT_SUNK_HUGE_BOAT){
        f->field[gData->row][gData->col] = FIELD_POSITION_HIT;
        f->hugeBoatLives = 0; // no more lives on huge boat
    }
    return ogPos; //returns starting position
}

//Initialize FieldGetBoatStates
uint8_t FieldGetBoatStates(const Field *f){
    int boatsLeft = (FIELD_BOAT_STATUS_SMALL|FIELD_BOAT_STATUS_MEDIUM|FIELD_BOAT_STATUS_LARGE
    |FIELD_BOAT_STATUS_HUGE); //bitwise or for when these boats appear
    
    if (f->smallBoatLives == 0){ //if equals to 0
        boatsLeft ^= FIELD_BOAT_STATUS_SMALL; //gives status of small boat
    }
    if (f->mediumBoatLives == 0){
        boatsLeft ^= FIELD_BOAT_STATUS_MEDIUM; //status of medium boat
    }
    if (f->largeBoatLives == 0) {
        boatsLeft ^= FIELD_BOAT_STATUS_LARGE; //Status for large boat
    }
    if (f->hugeBoatLives == 0) {
        boatsLeft ^= FIELD_BOAT_STATUS_HUGE; //Status for huge boat
    }
    return boatsLeft; //returns status of all the boats
}