#include "engine.c"
#define FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
int main () {
    struct Position pos = FENtoPosition(FEN);
    ApplyCastlingRights(FEN, &pos);
    printPosition(pos)rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1;
    printf("\n:%d", pos.wcastle);
    return 0;
}
