#include "engine.c"
#define FEN "8/8/8/3Q4/8/8/8/8 w - - 0 1"
int main () {
    struct Position pos = FENtoPosition(FEN);
    ApplyCastlingRights(FEN, &pos);
    //printPosition(pos);
    /*
    struct Move mymove;
    mymove.start.x = 2;
    mymove.start.y = 3;
    mymove.end.x = 5;
    mymove.end.y = 0;
    */
    //printf("\nValid: %d\n", validityCheck(pos, mymove));
    int counter = 0;
    struct Move* moves = getMoves(pos, &counter);
    printf("\ncounter: %d", counter);
   
    return 0;
}
