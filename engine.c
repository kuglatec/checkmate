#include "structs.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
struct Position FENtoPosition(char fen[256]) {
  struct Position position;
  memset(&position, 0, sizeof(struct Position)); // Initialize the struct
  int over = 0;
  const int len = strlen(fen);
  char rank = 7;  // Start from the bottom rank (White's first rank)
  char file = 0;
  int field = 0; // Track the current FEN field

  for (int i = 0; i < len; i++) {
    char nextchar = fen[i];

    if (over == 0) {
      if (isalpha(nextchar)) {
        position.board[file][rank] = nextchar;
        file++;
      }
      if (nextchar == '/') {
        rank--;
        file = 0;
      }
      if (isdigit(nextchar)) {
        int emptySquares = nextchar - '0';
        for (int j = 0; j < emptySquares; j++) {
          position.board[file][rank] = 'X';
          file++;
        }
      }
      if (nextchar == ' ') {
        over = 1;
        field++;
      }
    } else {
      // Handle fields other than piece placement
      switch (field) {
        case 1: // Active color
          position.player = (nextchar == 'w') ? 0 : 1;
        field++;
        break;
      }
    }
  }

  return position;
}
void ApplyCastlingRights(char fen[256], struct Position *position) {
  int spaceCount = 0;
  position->wcastle = 0;
  position->bcastle = 0;

  for (int i = 0; fen[i] != '\0'; i++) {
    if (fen[i] == ' ') {
      spaceCount++;
      if (spaceCount == 2) {
        i++;
        if (fen[i] == '-') {
          return;
        }
        while (fen[i] != ' ') {
          switch (fen[i]) {
            case 'K': position->wcastle |= 1; break;
            case 'Q': position->wcastle |= 2; break;
            case 'k': position->bcastle |= 1; break;
            case 'q': position->bcastle |= 2; break;
          }
          i++;
        }
        return;
      }
    }
  }
}
void printPosition(struct Position position) {
  for (int i = 0; i < 8; i++) {
    printf("\n");
    for (int j = 0; j < 8; j++) {
      printf("%c ", position.board[j][i]);
    }
  }
}



int isDiagonalMove(struct Move move) {
  return abs(move.start[0] - move.end[0]) == abs(move.start[1] - move.end[1]);
}
int isStraightMove(struct Move move) {
  return (move.start[0] == move.end[0]) || (move.start[1] == move.end[1]);
}

int moveValidityCheck(struct Position position, struct Move move) {
  switch (position.board[move.start[0]][move.start[1]]) {
    case 'Q':
      return queenValidityCheck(position, move);
  }
}

struct Square* getPathDiagonal(struct Move move) {

}

struct Square* getPathStraight(struct Move move) {
  
}

struct Square* getPath(struct Move move, int dir) {
  if (dir == 0) {
    return getPathStraight(move);
  }
  else if (dir == 1) {
    return getPathDiagonal(move);
  }
  else {
    printf("error, invalid direction");
  }
}
