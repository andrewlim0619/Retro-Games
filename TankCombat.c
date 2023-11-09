/* -------------------------------------------------------------------
    Programmer: Andrew Lim, Casey Duncan, Jayden Anderson
    Project: Tank Combat Source Code
    Goal: Porting from Atari 2600 to Atari 800 using C
    Compiler: CC65 Cross Compiler
   -------------------------------------------------------------------
*/

#include <atari.h>
#include <_antic.h>
#include <_atarios.h>
#include <peekpoke.h>
#include <conio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <joystick.h>

//Defining the 16 tank rotations 
#define NORTH               0
#define NORTH_15            1
#define NORTH_EAST          2
#define NORTH_60            3
#define EAST                4
#define EAST_15             5
#define SOUTH_EAST          6
#define EAST_60             7
#define SOUTH               8
#define SOUTH_15            9
#define SOUTH_WEST          10
#define SOUTH_60            11
#define WEST                12
#define WEST_15             13
#define NORTH_WEST          14
#define WEST_60             15

/*
 * <-------------------- GLOBAL VARIABLES --------------------> 
 */

//Different tank pictures to be printed
unsigned int tankPics[16][8] = {
        {8,8,107,127,127,127,99,99},        //NORTH
        {36,100,121,255,255,78,14,4},       //NORTH_15
        {25,58,124,255,223,14,28,24},       //NORTH_EAST
        {28,120,251,124,28,31,62,24},       //NORTH_60
        {0,252,252,56,63,56,252,252},       //EAST
        {24,62,31,28,124,251,120,28},       //EAST_15
        {25,58,124,255,223,14,28,24},       //SOUTH_EAST
        {4,14,78,255,255,121,36},           //EAST_60
        {99,99,127,127,127,107,8,8},        //SOUTH
        {32,112,114,255,255,158,38,36},     //SOUTH_15
        {152,92,62,255,251,112,56,24},      //SOUTH_WEST
        {24,124,248,56,62,223,30,56},       //SOUTH_60
        {0,63,63,28,252,28,63,63},          //WEST
        {56,30,223,62,56,248,124,24},       //WEST_15
        {152,92,62,255,251,112,56,24},      //NORTH_WEST
        {36,38,158,255,255,114,112,32}      //WEST_60
};

int i;

//Adresses
int bitMapAddress;
int PMBaseAddress;
int playerAddress;
int missileAddress;

//Horizontal Positions Registers
int *horizontalRegister_P0 = (int *)0xD000;
int *horizontalRegister_M0 = (int *)0xD004;
int *horizontalRegister_P1 = (int *)0xD001;
int *horizontalRegister_M1 = (int *)0xD005;

//Color-Luminance Registers
int *colLumPM0 = (int *)0x2C0;
int *colLumPM1 = (int *)0x2C1;

//Starting direction of each Players
unsigned int p0Direction = EAST;
unsigned int p1Direction = WEST;
unsigned char p0LastMove;
unsigned char p1LastMove;

//Starting vertical and horizontal position of players 
const int verticalStartP0 = 131;
const int horizontalStartP0 = 57;
const int verticalStartP1 = 387;
const int horizontalStartP1 = 190;

//Variables to track vertical and horizontal locations of players
int p0VerticalLocation = 131;
int p0HorizontalLocation = 57; 
int p1VerticalLocation = 387;
int p1HorizontalLocation = 190;

int m0LastHorizontalLocation;
int m0LastverticalLocation;
int m1LastHorizontalLocation;
int m1LastVerticalLocation;

/*
 * <-------------------- FUNCTION DECLARATIONS --------------------> 
 */
//functions to be implemented
void rearrangingDisplayList();
void initializeScore();
void createBitMap();
void enablePMGraphics();
void setUpTankDisplay();
void setUpMissile();

void movePlayers();
void fire(int tank);
void moveForward(int tank);
void moveBackward(int tank);
void checkCollision();
//functions to turn and update tank positions
void turnplayer(unsigned char turn, int player);
void updateplayerDir(int player);

/*
 * <-------------------- DRIVER MAIN --------------------> 
 */
int main() {
    joy_load_driver(joy_stddrv);        //Load joystick driver
    joy_install(joy_static_stddrv);     //Install joystick driver

    _graphics(18);      //Set default display to graphics 3 + 16 (+16 displays mode with graphics, eliminating the text window)
    rearrangingDisplayList();       //rearranging graphics 3 display list
    initializeScore();
    createBitMap();
    enablePMGraphics();
    setUpTankDisplay();
    //setUpMissile();

    while (true) {
        movePlayers();
        setUpMissile();
    }
    return 0;
}


/*
 * <-------------------- FUNCTION IMPLEMENTATIONS --------------------> 
 */
void rearrangingDisplayList() {
    unsigned int *DLIST_ADDRESS  = OS.sdlstl + OS.sdlsth*256;
    unsigned char *DLIST = (unsigned char *)DLIST_ADDRESS;

    // Write the Display List for Graphics Mode
    // Must contain a total of 192 Scan Lines for display list to function properly 
    unsigned char displayList[] = {
        DL_BLK8,  // 8 blank lines
        DL_BLK8,
        DL_BLK8,
        DL_LMS(DL_CHR20x16x2), 
        0xE0, 0x9C,  // Text Memory 
        DL_LMS(DL_MAP40x8x4),
        0x70,0x9E,  //Screen memory
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4, 
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4, 
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4, 
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_MAP40x8x4,
        DL_JVB,  // Jump and vertical blank
        0x4E, 0x9E  // Jump address (in this case, loop back to the start)
    };

    int i;

    for (i = 0; i < sizeof(displayList); i++) {
        POKE(DLIST + i, displayList[i]);
    }

    bitMapAddress = PEEK(DLIST+7) + PEEK(DLIST+8)*256;
}

void initializeScore() {
    //Temp code
    POKE(0x58,224);
    POKE(0X59,156);
    POKE(0X57,2);
    cputsxy(5, 0, "0");
    cputsxy(14, 0, "0");
}

void createBitMap() {
    //Making the top and bottom border
    for (i = 0; i < 10; i++)
    {
        POKE(bitMapAddress+i, 170);
        POKE(bitMapAddress+210+i, 170);
    }

    //Making the left border
    for (i = 10; i <= 200; i += 10)
    {
        POKE(bitMapAddress+i, 128);
    }

    //Making the right border
    for (i = 19; i <= 209; i += 10)
    {
        POKE(bitMapAddress+i, 2);
    }

    // <--- Making the sprites --->
    POKE(bitMapAddress+81, 40);
    POKE(bitMapAddress+131, 40);
    for (i = 91; i <= 121; i += 10)
    {
        POKE(bitMapAddress+i, 8);
    }

    POKE(bitMapAddress+88, 40);
    POKE(bitMapAddress+138, 40);
    for (i = 98; i <= 128; i += 10) 
    {
        POKE(bitMapAddress+i, 32);
    }

    for (i = 54; i <= 74; i += 10)
    {
        POKE(bitMapAddress+i, 2);
    }

    for (i = 55; i <= 75; i += 10) 
    {
        POKE(bitMapAddress+i, 128);
    }

    for (i = 144; i <= 164; i += 10) 
    {
        POKE(bitMapAddress+i, 2);
    }

    for (i = 145; i <= 165; i += 10) 
    {
        POKE(bitMapAddress+i, 128);
    }

    POKE(bitMapAddress+103, 168);
    POKE(bitMapAddress+113, 168);
    POKE(bitMapAddress+106, 42);
    POKE(bitMapAddress+116, 42);

    POKE(0x2C5, 26);    //Sets bitmap color to yellow
}

void enablePMGraphics() {
    POKE(0x22F, 62);                    //Enable Player-Missile DMA single line
    PMBaseAddress = PEEK(0x6A)-8;       //Get Player-Missile base address 
    POKE(0xD407, PMBaseAddress);        //Store Player-Missile base address in base register
    POKE(0xD01D, 3);                    //Enable Player-Missile DMA

    //Clear up Player Missile Character
    playerAddress = (PMBaseAddress * 256) + 1024;
    missileAddress = (PMBaseAddress * 256) + 768;
    
    //Clear up default built-in characters in Player's address
    for (i = 0; i < 512; i++) {
        POKE(playerAddress + i, 0);

        //Clear up default built-in characters in Missile's address
        if (i <= 256)
        {
            POKE(missileAddress + i, 0);
        }
    }
}

void setUpTankDisplay() {
    int counter = 0;

    //Set up player 0 tank 
    POKE(horizontalRegister_P0, 57);
    POKE(colLumPM0, 196);

    for (i = 131; i < 139; i++) {
        POKE(playerAddress+i, tankPics[NORTH_15][counter]);
        counter++;
    }
    counter = 0;

    //Set up player 1 tank
    POKE(horizontalRegister_P1, 190);
    POKE(colLumPM1, 116);

    for (i = 387; i < 395; i++) {
        POKE(playerAddress+i, tankPics[WEST][counter]);
        counter++;
    }
    counter = 0;
}

void setUpMissile() //Parameter must include (Tank Direction - for poking the missile initial position (at the tip of the barrel) and, their vertical location, and their horizontal position)
{   
    //Clear missile register before poking anything: Find a better way O(1)
    for (i = 0; i < 256; i++)
    {
        POKE(missileAddress + i, 0);
    }


    //Player 0 missile position for setups
    if (p0Direction == NORTH)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+4;
        m0LastverticalLocation = p0VerticalLocation;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+4);
        // POKE(missileAddress+p0VerticalLocation, 2);
    }
    else if (p0Direction == NORTH_15)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+5;
        m0LastverticalLocation = p0VerticalLocation;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+5);
        // POKE(missileAddress+p0VerticalLocation, 2);
    }
    else if (p0Direction == NORTH_EAST)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+7;
        m0LastverticalLocation = p0VerticalLocation;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+7);
        // POKE(missileAddress+p0VerticalLocation, 2);
    }
    else if (p0Direction == NORTH_60)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+7;
        m0LastverticalLocation = p0VerticalLocation+2;        
        // POKE(horizontalRegister_M0, p0HorizontalLocation+7);
        // POKE(missileAddress+p0VerticalLocation+2, 2);
    }
    else if (p0Direction == EAST)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+7;
        m0LastverticalLocation = p0VerticalLocation+4;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+7);
        // POKE(missileAddress+p0VerticalLocation+4, 2);        
    }
    else if (p0Direction == EAST_15)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+5;
        m0LastverticalLocation = p0VerticalLocation+7;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+5);
        // POKE(missileAddress+p0VerticalLocation+7, 2);
    }
    else if (p0Direction == SOUTH_EAST)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+7;
        m0LastverticalLocation = p0VerticalLocation+7;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+7);
        // POKE(missileAddress+p0VerticalLocation+7, 2);        
    }
    else if (p0Direction == EAST_60)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+7;
        m0LastverticalLocation = p0VerticalLocation+5;
        // POKE(horizontalRegister_M0, p0HorizontalLocation+7);
        // POKE(missileAddress+p0VerticalLocation+5, 2);            
    } 
    else if (p0Direction == SOUTH)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+4;
        m0LastverticalLocation = p0VerticalLocation+7;
    }
    else if (p0Direction == SOUTH_15)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+2;
        m0LastverticalLocation = p0VerticalLocation+7;
    }
    else if (p0Direction == SOUTH_WEST)
    {
        m0LastHorizontalLocation = p0HorizontalLocation;
        m0LastverticalLocation = p0VerticalLocation+7;
    }
    else if (p0Direction == SOUTH_60)
    {
        m0LastHorizontalLocation = p0HorizontalLocation;
        m0LastverticalLocation = p0VerticalLocation+5;
    }
    else if (p0Direction == WEST)
    {
        m0LastHorizontalLocation = p0HorizontalLocation;
        m0LastverticalLocation = p0VerticalLocation+4;        
    }
    else if (p0Direction == WEST_15)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+2;
        m0LastverticalLocation = p0VerticalLocation;
    }
    else if (p0Direction == NORTH_WEST)
    {
        m0LastHorizontalLocation = p0HorizontalLocation;
        m0LastverticalLocation = p0VerticalLocation;
    }
    else if (p0Direction == WEST_60)
    {
        m0LastHorizontalLocation = p0HorizontalLocation+2;
        m0LastverticalLocation = p0VerticalLocation+2;
    }

    //If missile for player 0 is fired
}

//moving based off of joystick input, or firing the tank if the player chooses
void movePlayers(){
    //joystick code
    unsigned char player1move = joy_read(JOY_1);
    //unsigned char player2move = joy_read(JOY_2);

    if(JOY_BTN_1(player1move)) fire(1);
    else if(JOY_UP(player1move)) moveForward(1);
    else if(JOY_DOWN(player1move)) moveBackward(1);
    else if(JOY_LEFT(player1move) || JOY_RIGHT(player1move)) turnplayer(player1move, 1);

    //moving player 2
    //if(JOY_BTN_1(player2move)) fire(2);
    //else if(JOY_UP(player2move)) moveForward(2);
    //else if(JOY_DOWN(player2move)) moveBackward(2);
    //else if(JOY_LEFT(player2move) || JOY_RIGHT(player2move)) turnplayer(player2move, 2);
}

//rotating the player
void turnplayer(unsigned char turn, int player){
    //for player 1
    if(player == 1){
        //handling edge cases
        if(p0Direction == WEST_60 && JOY_RIGHT(turn)){
            p0Direction = NORTH;
        }
        else if(p0Direction == NORTH && JOY_LEFT(turn)){
            p0Direction = WEST_60;
        }
        //if the joystick is left,
        else if(JOY_LEFT(turn)){
            p0Direction = p0Direction - 1;
        }
        //if the joystick is right
        else if(JOY_RIGHT(turn)){
            p0Direction = p0Direction + 1;
        }
        updateplayerDir(1);
    }
//    //for player 2
//    else if(player == 2){
//        if(p1Direction == WEST_60 && turn == 'R'){
//            p1Direction = NORTH;
//        }
//        else if(p1Direction == NORTH && turn == 'L'){
//            p1Direction = WEST_60;
//        }
//            //if the joystick is left,
//        else if(turn == 'L'){
//            p1Direction = p1Direction - 1;
//        }
//            //if the joystick is right
//        else if(turn == 'R'){
//            p1Direction = p1Direction + 1;
//        }
//        updateplayerDir(2);
//    }
}

//updating the player's orientation, or position
void updateplayerDir(int player){
    //updating player 1
    if(player == 1){
        if(p0Direction == SOUTH_WEST || p0Direction == SOUTH_EAST){
            int counter = 7;
            for(i = p0VerticalLocation; i < p0VerticalLocation + 8; i++){
                POKE(playerAddress + i, tankPics[p0Direction][counter]);
                counter--;
            }
        }
        else{
            int counter = 0;
            for(i = p0VerticalLocation; i < p0VerticalLocation + 8; i++){
                POKE(playerAddress + i, tankPics[p0Direction][counter]);
                counter++;
            }
        }
    }

    //updating player 2
    else if(player == 2){
        if(p1Direction == SOUTH_15 || p1Direction == SOUTH_60 || p1Direction == SOUTH_WEST || p1Direction == EAST_15 || p1Direction == SOUTH_EAST || p1Direction == EAST_60){
            int counter = 7;
            for(i = p1VerticalLocation; i < p1VerticalLocation + 8; i++){
                POKE(playerAddress + i, tankPics[p1Direction][counter]);
                counter--;
            }
        }
        else{
            int counter = 0;
            for(i = p1VerticalLocation; i < p1VerticalLocation + 8; i++){
                POKE(playerAddress + i, tankPics[p1Direction][counter]);
                counter++;
            }
        }
    }

    //checking to see if the movement caused a collision, is it needed here?
    checkCollision();
}

//add a check to the collision registers
void checkCollision(){

}

//move the tank forward
void moveForward(int tank){
    //moving forward tank 1
    if(tank == 1){
        //movement for north
        if(p0Direction == NORTH){
            POKE(playerAddress+(p0VerticalLocation+7), 0);
            p0VerticalLocation--;

        }
        //movement for south
        if(p0Direction == SOUTH){
            POKE(playerAddress+p0VerticalLocation, 0);
            p0VerticalLocation++;
        }
        //movement north-ish cases
        if(p0Direction == NORTH_15 || p0Direction == NORTH_60 || p0Direction == NORTH_EAST || p0Direction == WEST_15 || p0Direction == NORTH_WEST || p0Direction == WEST_60){
            int x = 0;
            int y = 0;
            //X ifs
            if(p0Direction == NORTH_EAST || p0Direction == NORTH_WEST || p0Direction == NORTH_15 || p0Direction == WEST_60) x = 1;
            if(p0Direction == NORTH_60 || p0Direction == WEST_15) x = 2;
            //Y ifs
            if(p0Direction == NORTH_EAST || p0Direction == NORTH_WEST || p0Direction == NORTH_60 || p0Direction == WEST_15) y = 1;
            if(p0Direction == NORTH_15 || p0Direction == WEST_60) y = 2;
            if(p0Direction < 4) p0HorizontalLocation = p0HorizontalLocation + x;
            else p0HorizontalLocation = p0HorizontalLocation - x;
            //x possible outcomes = -2, -1, 1, 2
            //y possible outcomes = 2, 1

            POKE(playerAddress+(p0VerticalLocation +7), 0);
            POKE(playerAddress+(p0VerticalLocation +6), 0);
            p0VerticalLocation = p0VerticalLocation - y;
            updateplayerDir(1);
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        //movement south-ish cases
        if(p0Direction == SOUTH_15 || p0Direction == SOUTH_60 || p0Direction == SOUTH_WEST || p0Direction == EAST_15 || p0Direction == SOUTH_EAST || p0Direction == EAST_60){
            int x = 0;
            int y = 0;
            //X ifs
            if(p0Direction == SOUTH_WEST || p0Direction == SOUTH_EAST || p0Direction == SOUTH_15 || p0Direction == EAST_60) x = 1;
            if(p0Direction == SOUTH_60 || p0Direction == EAST_15) x = 2;
            //Y ifs
            if(p0Direction == SOUTH_WEST || p0Direction == SOUTH_EAST || p0Direction == SOUTH_60 || p0Direction == EAST_15) y = 1;
            if(p0Direction == SOUTH_15 || p0Direction == EAST_60) y = 2;
            if(p0Direction < 8) p0HorizontalLocation = p0HorizontalLocation + x;
            else p0HorizontalLocation = p0HorizontalLocation - x;
            //x possible outcomes = -2, -1, 1, 2
            //y possible outcomes = 2, 1

            POKE(playerAddress+(p0VerticalLocation), 0);
            POKE(playerAddress+(p0VerticalLocation +1), 0);
            p0VerticalLocation = p0VerticalLocation + y;
            updateplayerDir(1);
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        //movement west
        if(p0Direction == WEST){
            p0HorizontalLocation--;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        //movement east
        if(p0Direction == EAST){
            p0HorizontalLocation++;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        updateplayerDir(1);
    }

    //moving forward tank 2
    else if(tank == 2){

    }
}

//move the tank backward
void moveBackward(int tank){
    //movement for tank 1
    if(tank == 1){
        //movement for north
        if(p0Direction == NORTH){
            POKE(playerAddress+p0VerticalLocation, 0);
            p0VerticalLocation++;
        }
        //movement for south
        if(p0Direction == SOUTH){
            POKE(playerAddress+(p0VerticalLocation+7), 0);
            p0VerticalLocation--;
        }
        //movement north-ish cases
        if(p0Direction == NORTH_15 || p0Direction == NORTH_60 || p0Direction == NORTH_EAST || p0Direction == WEST_15 || p0Direction == NORTH_WEST || p0Direction == WEST_60){
            int x = 0;
            int y = 0;
            //X ifs
            if(p0Direction == NORTH_EAST || p0Direction == NORTH_WEST || p0Direction == NORTH_15 || p0Direction == WEST_60) x = 1;
            if(p0Direction == NORTH_60 || p0Direction == WEST_15) x = 2;
            //Y ifs
            if(p0Direction == NORTH_EAST || p0Direction == NORTH_WEST || p0Direction == NORTH_60 || p0Direction == WEST_15) y = 1;
            if(p0Direction == NORTH_15 || p0Direction == WEST_60) y = 2;
            if(p0Direction > 4) p0HorizontalLocation = p0HorizontalLocation + x;
            else p0HorizontalLocation = p0HorizontalLocation - x;
            //x = -2, -1, 1, 2
            //y = 2, 1

            POKE(playerAddress+(p0VerticalLocation), 0);
            POKE(playerAddress+(p0VerticalLocation + 1), 0);
            p0VerticalLocation = p0VerticalLocation + y;
            updateplayerDir(1);
            POKE(horizontalRegister_P0, p0HorizontalLocation);

        }
        //movement south-ish cases
        if(p0Direction == SOUTH_15 || p0Direction == SOUTH_60 || p0Direction == SOUTH_WEST || p0Direction == EAST_15 || p0Direction == SOUTH_EAST || p0Direction == EAST_60){
            int x = 0;
            int y = 0;
            //X ifs
            if(p0Direction == SOUTH_WEST || p0Direction == SOUTH_EAST || p0Direction == SOUTH_15 || p0Direction == EAST_60) x = 1;
            if(p0Direction == SOUTH_60 || p0Direction == EAST_15) x = 2;
            //Y ifs
            if(p0Direction == SOUTH_WEST || p0Direction == SOUTH_EAST || p0Direction == SOUTH_60 || p0Direction == EAST_15) y = 1;
            if(p0Direction == SOUTH_15 || p0Direction == EAST_60) y = 2;
            if(p0Direction < 8) p0HorizontalLocation = p0HorizontalLocation - x;
            else p0HorizontalLocation = p0HorizontalLocation + x;
            //x possible outcomes = -2, -1, 1, 2
            //y possible outcomes = 2, 1

            POKE(playerAddress+(p0VerticalLocation + 7), 0);
            POKE(playerAddress+(p0VerticalLocation +6), 0);
            p0VerticalLocation = p0VerticalLocation - y;
            updateplayerDir(1);
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        //movement west
        if(p0Direction == WEST){
            p0HorizontalLocation++;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        //movement east
        if(p0Direction == EAST){
            p0HorizontalLocation--;
            POKE(horizontalRegister_P0, p0HorizontalLocation);
        }
        updateplayerDir(1);
    }

    if(tank == 2){

    }
}

//fire a projectile from the tank
void fire(int tank){

}
