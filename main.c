#include "engine.c"
#include <stdio.h>
#define FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//#define FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
int main () {
  struct Position pos = FENtoPosition(FEN);
  ApplyCastlingRights(FEN, &pos);
  struct Node node;
  node.position = pos;
  buildTree(&node, 5);
  return 0;
}
