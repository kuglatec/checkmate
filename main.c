#include "engine.c"
#define FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
int main () {
    struct Position pos = FENtoPosition(FEN);
    ApplyCastlingRights(FEN, &pos);
    printPosition(pos);
    struct Move myMove;
    myMove.start[0] = 1;
    myMove.start[1] = 1;
    myMove.end[0] = 6;
    myMove.end[1] = 2;
    int v = validityCheck(pos, myMove);
    return 0;
}
