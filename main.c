#include "engine.c"
#include <stdio.h>
#define FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
/*
int main () {
  struct Position pos = FENtoPosition(FEN);
  ApplyCastlingRights(FEN, &pos);
  ApplyEnpassantTarget(FEN, &pos);
    struct Node node;
  node.position = pos;
  buildTree(&node, 5);
 // printf("\nTree built with %d children\n", node.children[0].nchildren;

printTree(node, 5);
  return 0;
}
*/
int main() {
  initZobrist();

  struct Position pos = FENtoPosition(FEN);
  ApplyCastlingRights(FEN, &pos);
  ApplyEnpassantTarget(FEN, &pos);

  struct hashSet* seen = createHashSet(10000); // Start with space for 10k positions

  struct Node node;
  node.position = pos;
  buildTreeWithHashCheck(&node, 6, seen);

  printf("Total unique positions seen: %zu\n", seen->size);

  freeHashSet(seen);
  return 0;
}
