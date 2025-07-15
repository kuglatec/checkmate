#include "engine.c"
#define FEN "8/8/8/8/1r6/P7/8/8 w - - 0 1"
int main () {
    struct Position pos = FENtoPosition(FEN);
    ApplyCastlingRights(FEN, &pos);
//    printPosition(pos);
    struct Move myMove;
    myMove.start[0] = 0;
    myMove.start[1] = 2;
    myMove.end[0] = 1;
    myMove.end[1] = 3;
    int v = validityCheck(pos, myMove);
    printf("\nresult: %d", v);
    return 0;
}
