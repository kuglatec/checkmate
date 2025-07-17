#include "structs.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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
  return abs(move.start.x - move.end.x) == abs(move.start.y - move.end.y);
}

int isStraightMove(struct Move move) {
  return (move.start.x == move.end.x) || (move.start.y == move.end.y);
}

void addSquareVectors(struct Square* sq, struct Square direction) {
  sq->x += direction.x;
  sq->y += direction.y;
}

void multiplyVector(struct Square* vector, int factor) {
  vector->x = vector->x * factor;
  vector->y = vector->y * factor;
}

struct Path getPathDiagonal(struct Move move) {
  int dx = move.end.x - move.start.x;
  int dy = move.end.y - move.start.y;

  size_t len = abs(dx) - 1;

  struct Square direction;
  direction.x = (dx > 0) ? 1 : -1;
  direction.y = (dy > 0) ? 1 : -1;

  struct Square sq = move.start;
  struct Square* squares = calloc(len, sizeof(struct Square));

  for (int i = 0; i < len; i++) {
    addSquareVectors(&sq, direction);
    squares[i] = sq;
  }

  struct Path pth;
  pth.len = len;
  pth.squares = squares;
  return pth;
}
struct Path getPathStraight(struct Move move) {
  size_t len;
  if (move.start.x == move.end.x) {
    len = abs(move.start.y - move.end.y) - 1;
  } else {
    len = abs(move.start.x - move.end.x) - 1;
  }
  
  struct Square direction;
  direction.x = (move.end.x - move.start.x) / (len + 1);
  direction.y = (move.end.y - move.start.y) / (len + 1);
  
  
  
  struct Square sq = move.start;
  
  struct Square* squares = (struct Square*)calloc(len, sizeof(struct Square));
  
  for (int i = 0; i < len; i++) {
    addSquareVectors(&sq, direction);
    squares[i] = sq;
  }
  
  struct Path pth;
  pth.squares = squares;
  pth.len = len;
  return pth;
}

char getPiece(struct Square sq, struct Position pos) {
  return pos.board[sq.x][sq.y];
}

struct Path getPath(struct Move move, int dir) {
  if (dir == 0) {
    return getPathStraight(move);
  }
  else if (dir == 1) {
    return getPathDiagonal(move);
  }
}

int qrsvc(struct Position position, struct Move move, int owner) { //Queen-Rook-Straight-Validity-Check
  struct Path path = getPath(move, 0);
  for (int i = 0; i < path.len; i++) {
    if (getPiece(path.squares[i], position) != 'X') {
      return 0; //some piece is blocking the path
    }
  }
  return 1;
}

int qbdvc(struct Position position, struct Move move, int owner) { //Queen-Bishop-diagonal-validitiy check
    struct Path path = getPath(move, 1);
    for (int i = 0; i < path.len; i++) {
   
    if (getPiece(path.squares[i], position) != 'X') {
      return 0; //some piece is blocking the path
    }
  }
  return 1;
}


int kvc(struct Position position, struct Move move, int owner) {
 // toDo: implement check logic
 
  // if ((abs(move.start.x - move.end.x) <= 1) && (abs(move.start.y - move.end.y) <= 1)) {
 //   return 1;
 // }
  return 1;
}

int checkCastling(struct Position position, struct Move move, int owner) {
  // ToDo: move generation
}

int pvc(struct Position position, struct Move move, int owner) {
  if (move.start.x == move.end.x) { // Pawn moves straight    
    struct Square dsquare = move.end;
    
    if (getPiece(dsquare, position) != 'X') {
      return 0;
    }
    
    if (abs(move.start.y - move.end.y) == 2) {
        if (move.start.y == 1 || move.start.y == 6){
          return qrsvc(position, move, owner);
        }
        else {
          return 0;
        }
    }
    else if (abs(move.start.y - move.end.y) == 1) {
      return 1;
    }
  }

//###################################################

  else if ((abs(move.start.x - move.end.x) == 1) && (abs(move.start.y - move.end.y) == 1)){ // Pawn captures diagonal
    struct Square dsquare = move.end;
    
    if (isupper(getPiece(dsquare, position)) != owner) {
      return 1;
    }

  }
  return 0;
}

int validityCheck(struct Position position, struct Move move) {
  char dp = position.board[move.end.x][move.end.y];
  char sp = position.board[move.start.x][move.start.y];
  int owner = isupper(sp);
  
  
  if (isupper(dp) == owner && dp != 'X') {
    return 0; // some piece from the same player is blocking the destination square, making the move illegal.
  }
  
  if (sp == 'P' || sp == 'p') {
    return pvc(position, move, owner);
  }
  
  if (isDiagonalMove(move)) {
    switch (sp)
    {
    case 'Q':
    case 'q':
    case 'B':
    case 'b':
    return qbdvc(position, move, owner);  
    break;
    }
  }
  else if (isStraightMove(move)) {
    switch (sp)
    {
    case 'Q':
    case 'q':
    case 'R':
    case 'r':
      return qrsvc(position, move, owner);
      break;
    
    default:
      break;
    }
  }
  else if (sp == 'N' || sp == 'n') {
    return 1;
  }
  
  return 0;
}



void getDiagonalSquares(struct Path* lines, struct Square start) {

    int count = lines->len;
    int dx[] = {-1, -1, 1, 1};
    int dy[] = {-1, 1, -1, 1};
    
    for (int dir = 0; dir < 4; dir++) {
        int x = start.x + dx[dir];
        int y = start.y + dy[dir];
        
        while (x >= 0 && x < 8 && y >= 0 && y < 8 && count < 28) {
            lines->squares[count].x = x;
            lines->squares[count].y = y;
            count++;
            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len = count;
}

void getStraightSquares(struct Path* lines, struct Square start) {
    int count = lines->len;
    int dx[] = {0, 0, 1, -1}; 
    int dy[] = {1, -1, 0, 0};
    
    for (int dir = 0; dir < 4; dir++) {
        int x = start.x + dx[dir];
        int y = start.y + dy[dir];
        
        while (x >= 0 && x < 8 && y >= 0 && y < 8 && count < 28) {
            lines->squares[count].x = x;
            lines->squares[count].y = y;
            count++;
            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len += count;
}

void getPawnSquares(struct Path* lines, struct Square piece, struct Position position) {
  int dir;
  if (isupper(getPiece(piece, position)) == 1) { // Pawn is white
      dir = -1;
  }
  else {
    dir = 1;
  }
  lines->squares[0].x = piece.x;
  lines->squares[0].y = piece.y + dir;
  lines->squares[1].x = piece.x;
  lines->squares[1].y = piece.y + (dir * 2);
  lines->squares[2].x = piece.x + 1;
  lines->squares[2].y = piece.y + dir;
  lines->squares[3].x = piece.x - 1;
  lines->squares[3].y = piece.y + dir;
  lines->len = 4;
}

void getKingSquares(struct Path* lines, struct Square piece, struct Position position) {

}

void genMoves(struct Move* moves, struct Position position, struct Square piece, int* counter) {
  struct Path lines;
  lines.squares = malloc(sizeof(struct Square) * 28);
  lines.len = 0;
  struct Move mv;

  switch (getPiece(piece, position)) {
    case 'b':
    case 'B':
      getDiagonalSquares(&lines, piece);
      break;
    case 'r':
    case 'R':
      getStraightSquares(&lines, piece);
      break;

    case 'p':
    case 'P':
      getPawnSquares(&lines, piece, position);
      break;

    case 'K':
    case 'k':
      getKingSquares(&lines, piece, position);
    
    case 'Q':
    case 'q':
      getStraightSquares(&lines, piece);
      getDiagonalSquares(&lines, piece);
  }

  for (int i = 0; i < lines.len; i++) {
    mv.start = piece;
    mv.end = lines.squares[i];
    if (validityCheck(position, mv)) {
      moves[*counter] = mv;
      (*counter)++;
    }
  }
  
  free(lines.squares);
}

  

struct Move* getMoves(struct Position position, int* counter) {
  struct Move* mvs = (struct Move*)calloc(218, sizeof(struct Move));
  struct Square sq;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sq.x = i;
      sq.y = j;
      if (position.player == isupper(getPiece(sq, position)) || getPiece(sq, position) == 'X') {
        continue;
      }
      genMoves(mvs, position, sq, counter);
    }
  }
  return mvs;
}
