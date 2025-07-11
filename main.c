#include "engine.c"
#define FEN "8/8/8/8/8/8/8/B7 w - - 0 1"
int main () {
    struct Position pos = FENtoPosition(FEN);
    ApplyCastlingRights(FEN, &pos);
//    printPosition(pos);
    struct Move myMove;
    myMove.start[0] = 0;
    myMove.start[1] = 0;
    myMove.end[0] = 1;
    myMove.end[1] = 1;
    int v = validityCheck(pos, myMove);
    return 0;
}
