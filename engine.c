#include "structs.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
struct Position FENtoPosition (char fen[256]) {
  int over = 0;
  const int len = strlen(fen);
  char rank = 0;
  char row = 0;
  struct Position position;
  for (int i = 0; i < len; i++) {
    char nextchar = fen[i];
    if (isalpha(nextchar)) {
      if (over == 0) {position.board[row][rank] = nextchar; row++;}
      else {
        switch (nextchar) {
          case 'w':
            position.player = 0;
          case 'b':
            position.player = 1;
        }
      }

    }
    if (nextchar == '/') {
      rank++;
      row = 0;
    }

    if (isdigit(nextchar)) {
      const int nrow = row;
      int emptySquares = nextchar - '0';  // Convert char digit to int
      while (row < nrow + emptySquares) {
        position.board[row][rank] = 'X';
        row++;
      }
    }
    if (nextchar == ' ') {
      over = 1;
    }
  }

  return position;
}
void printPosition(struct Position position) {
  for (int i = 0; i < 8; i++) {
    printf("\n");
    for (int j = 0; j < 8; j++) {
      printf("%c ", position.board[j][i]);
    }
  }
}