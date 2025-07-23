#include <string.h>
#include "structs.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int dbg = 1;

char wpromotions[4] = "QRBN";
char bpromotions[4] = "qrbn";

int isThreatened(struct Position* position, struct Square square);
void printPosition(struct Position position);


struct Position copyPosition(struct Position* original) {
    struct Position copy;
    memcpy(&copy, original, sizeof(struct Position));
    return copy;
}

struct Position FENtoPosition(const char* fen) {
  struct Position position;
  for (int i=0; i<8; i++) {
    for (int j=0; j<8; j++) {
      position.board[i][j] = 'X';
    }
  }
  int over = 0;
  const int len = strlen(fen);
  char rank = 7;  
  char file = 0;  
  int field = 0; 

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
        file += emptySquares;
      }
      if (nextchar == ' ') {
        over = 1;
        field++;
      }
    } else {
      switch (field) {
        case 1: 
          position.player = (nextchar == 'w') ? 0 : 1;
        field++;
        break;
      }
    }
  }

  return position;
}

void ApplyCastlingRights(const char* fen, struct Position *position) {
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

void printMoves(struct Move* moves, int len) {
  for (int i = 0; i < len; i++) {
    printf("\n%d|%d -> %d|%d", moves[i].start.x, moves[i].start.y, moves[i].end.x, moves[i].end.y);
    printf("\n");
  }
}

void printSquares(struct Square* squares, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("\n%d|%d", squares[i].x, squares[i].y);
  }
}

void promote(struct Position* position, struct Move move) {
  position->board[move.end.x][move.end.y] = move.promotion;
}

void printPosition(struct Position position) {
  printf("\n");
  for (int i = 7; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      printf("%c ", (unsigned char)position.board[j][i]);
    }
    printf("\n");
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

  size_t len = abs(dx) > 1 ? abs(dx) - 1 : 0;

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
    len = abs(move.start.y - move.end.y) > 1 ? abs(move.start.y - move.end.y) - 1 : 0;
  } else {
    len = abs(move.start.x - move.end.x) > 1 ? abs(move.start.x - move.end.x) - 1 : 0;
  }
  
  struct Square direction;
  direction.x = (move.end.x > move.start.x) ? 1 : (move.end.x < move.start.x) ? -1 : 0;
  direction.y = (move.end.y > move.start.y) ? 1 : (move.end.y < move.start.y) ? -1 : 0;
  
  
  
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

int incheck(struct Position* position) {
  int player_to_check = position->player;
  char king_to_find = (player_to_check == 0) ? 'K' : 'k';
  struct Square kingSquare = {-1, -1}; 

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (position->board[i][j] == king_to_find) {
        kingSquare.x = i;
        kingSquare.y = j;
        break;
      }
    }
    if (kingSquare.x != -1) break;
  }

  
  if (kingSquare.x == -1) {
    return 0; 
  }
  
  position->player = (player_to_check == 0) ? 1 : 0;

  int threatened = isThreatened(position, kingSquare);
  printf("\nIn incheck, threatened value: %d\n", threatened); 
  // Restore player
  position->player = player_to_check;

  return threatened;
}

void makeMove(struct Position* position, struct Move move, int special) { // special: 0 = normal move, 1 = castling 2 = e.p.
  if (!special){
    char piece = position->board[move.start.x][move.start.y];
    position->board[move.start.x][move.start.y] = 'X';
    position->board[move.end.x][move.end.y] = piece;
  }
  else if (special == 1) { // Castling
  }
  else if (special == 2) { // En passant
    char piece = position->board[move.start.x][move.start.y];
    position->board[move.start.x][move.start.y] = 'X';
    position->board[move.end.x][move.end.y] = piece;
    if (piece == 'P') {
      position->board[move.end.x][move.end.y - 1] = 'X'; 
    } else {
      position->board[move.end.x][move.end.y + 1] = 'X';
    }
  }
  if (move.promotes) {
    promote(position, move);
  }
}


char getPiece(struct Square sq, struct Position* pos) {
  return pos->board[sq.x][sq.y];
}

struct Path getPath(struct Move move, int dir) {
  if (dir == 0) {
    return getPathStraight(move);
  }
  else if (dir == 1) {
    return getPathDiagonal(move);
  }
  struct Path pth;
  pth.len = 0;
  pth.squares = NULL;
  return pth;
}

int qrsvc(struct Position* position, struct Move move, int owner) { //Queen-Rook-Straight-Validity-Check
  
  struct Path path = getPath(move, 0);
  for (int i = 0; i < path.len; i++) {
    if (getPiece(path.squares[i], position) != 'X') {
      free(path.squares);
      return 0;
    }
  }
  free(path.squares);
  return 1;
}

int qbdvc(struct Position* position, struct Move move, int owner) { //Queen-Bishop-diagonal-validitiy check
    struct Path path = getPath(move, 1);
    for (int i = 0; i < path.len; i++) {
   
    if (getPiece(path.squares[i], position) != 'X') {
      free(path.squares);
      return 0; 
    }
  }
  free(path.squares);
  return 1;
}


int kvc(struct Position* position, struct Move move, int owner) {
  
  if (abs(move.start.x - move.end.x) > 1) { //castling
    
  }

  int opponent = (owner == 0) ? 1 : 0;

  int original_player = position->player;
  position->player = opponent;

  int threatened = isThreatened(position, move.end);

  position->player = original_player;


  return !threatened;
}

int pvc(struct Position* position, struct Move move, int owner) {
  int dir = (owner == 0) ? 1 : -1; // White (0) moves up (y+1), Black (1) moves down (y-1)

  if (move.start.x == move.end.x) { // Pawn moves straight    
    if (getPiece(move.end, position) != 'X') {
      return 0; // Cannot move forward to an occupied square
    }
    
    if (move.end.y - move.start.y == dir) { // Normal 1-square move
        return 1;
    }

    if (move.end.y - move.start.y == 2 * dir) { // Double move from start
        int start_rank = (owner == 0) ? 1 : 6;
        if (move.start.y == start_rank) {
            struct Move singleMove = move;
            singleMove.end.y = move.start.y + dir;
            return qrsvc(position, singleMove, owner); // Check path is clear
        }
    }
    return 0;
  }
  else if ((abs(move.start.x - move.end.x) == 1) && (move.end.y - move.start.y == dir)){ // Pawn captures diagonal
    char dpiece = getPiece(move.end, position);
    if (dpiece == 'X') {
      return 0;
    }
    if (!!isupper(dpiece) == owner) {
        return 0; 
    }
    return 1; 
  }
  return 0;
}


int validityCheck(struct Position* position, struct Move move, int skip_king_check) {
  if (move.end.x < 0 || move.end.x > 7 || move.end.y < 0 || move.end.y > 7) {
    return 0; 
  }
  if (!skip_king_check) {
    struct Position tempPosition = copyPosition(position);
    makeMove(&tempPosition, move, 0);
    if (incheck(&tempPosition)) {
      printf("\nMove %d|%d -> %d|%d puts king in check, invalid.\n", move.start.x, move.start.y, move.end.x, move.end.y);
      return 0; 
    }
  }

  char sp = position->board[move.start.x][move.start.y];
  if (sp == 'X') {
    return 0; 
  }
  
  int owner = !!isupper((unsigned char)sp) ? 0 : 1; 
  if (owner != position->player) {
    return 0; 
  }
  
  char dp = position->board[move.end.x][move.end.y];
  if (dp != 'X' && (!!isupper((unsigned char)dp) ? 0 : 1) == owner) {
      return 0; 
  }

  char piece_type = tolower(sp);

  switch(piece_type) {
    case 'p':
      return pvc(position, move, owner);
    case 'k':
      return kvc(position, move, owner);
    case 'q':
      if (isDiagonalMove(move)) return qbdvc(position, move, owner);
      if (isStraightMove(move)) return qrsvc(position, move, owner);
      break;
    case 'b':
      if (isDiagonalMove(move)) return qbdvc(position, move, owner);
      break;
    case 'r':
      if (isStraightMove(move)) return qrsvc(position, move, owner);
      break;
    case 'n':
      // Knight move logic: L-shape (2 squares in one direction, 1 in another)
      return (abs(move.start.x - move.end.x) == 1 && abs(move.start.y - move.end.y) == 2) ||
             (abs(move.start.x - move.end.x) == 2 && abs(move.start.y - move.end.y) == 1);
  }
  printf("\nreturn 0\n");
  return 0;
}







void getDiagonalSquares(struct Path* lines, struct Square start, struct Position* position) {
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
            
            struct Square current_square = {x, y};
            if (getPiece(current_square, position) != 'X') {
                break; 
            }
            
            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len = count;
}

void getStraightSquares(struct Path* lines, struct Square start, struct Position* position) {
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

            struct Square current_square = {x, y};
            if (getPiece(current_square, position) != 'X') {
                break; 
            }
            
            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len = count;
}

void getPawnSquares(struct Path* lines, struct Square piece, struct Position* position) {
  int dir = (!!isupper(getPiece(piece, position))) ? 1 : -1;
  int count = 0;

  // 1 square forward
  int x = piece.x;
  int y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    struct Square one_forward = {x, y};
    if (getPiece(one_forward, position) == 'X') {
        lines->squares[count].x = x;
        lines->squares[count].y = y;
        count++;

        // 2 squares forward from starting position
        int start_rank = (dir == 1) ? 1 : 6;
        if (piece.y == start_rank) {
            x = piece.x;
            y = piece.y + (2 * dir);
            if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                struct Square two_forward = {x, y};
                if (getPiece(two_forward, position) == 'X') {
                    lines->squares[count].x = x;
                    lines->squares[count].y = y;
                    count++;
                }
            }
        }
    }
  }

  // Capture right
  x = piece.x + 1;
  y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    lines->squares[count].x = x;
    lines->squares[count].y = y;
    count++;
  }

  // Capture left
  x = piece.x - 1;
  y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    lines->squares[count].x = x;
    lines->squares[count].y = y;
    count++;
  }

  lines->len = count;
}

void getKingSquares(struct Path* lines, struct Square piece, struct Position* position) {
  lines->len = 0;
  int dx[] = {0, 0, 1, -1, 1, -1, 1, -1};
  int dy[] = {1, -1, 0, 0, 1, 1, -1, -1};
  for (int i = 0; i < 8; i++) {
    int x = piece.x + dx[i];
    int y = piece.y + dy[i];
    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
      lines->squares[lines->len].x = x;
      lines->squares[lines->len].y = y;
      lines->len++;
    }
  }
  if (position->player == 0) { 
    if (position->wcastle & 1) { 
      lines->squares[lines->len].x = 6; 
      lines->squares[lines->len].y = 0;
      lines->len++;
    }
    if (position->wcastle & 2) { 
      lines->squares[lines->len].x = 2; 
      lines->squares[lines->len].y = 0;
      lines->len++;
    }
  } else { 
    if (position->bcastle & 1) { 
      lines->squares[lines->len].x = 6; 
      lines->squares[lines->len].y = 7;
      lines->len++;
    }
    if (position->bcastle & 2) { 
      lines->squares[lines->len].x = 2; 
      lines->squares[lines->len].y = 7;
      lines->len++;
    }
  }
}




void getKnightSquares(struct Path* lines, struct Square piece) {
    lines->len = 0;
    int dx[] = {1, 1, 2, 2, -1, -1, -2, -2};
    int dy[] = {2, -2, 1, -1, 2, -2, 1, -1};
    for (int i = 0; i < 8; i++) {
        int x = piece.x + dx[i];
        int y = piece.y + dy[i];
        if (x >= 0 && x < 8 && y >= 0 && y < 8) {
            lines->squares[lines->len].x = x;
            lines->squares[lines->len].y = y;
            lines->len++;
        }
    }
}

void genMoves(struct Move* moves, struct Position* position, struct Square piece, int* counter, int skip_king_check) {
  struct Path lines;
  lines.squares = malloc(sizeof(struct Square) * 28);
  lines.len = 0;
  struct Move mv;

  char piece_type = tolower(getPiece(piece, position));

  switch (piece_type) {
    case 'b':
      getDiagonalSquares(&lines, piece, position);
      break;
    case 'r':
      getStraightSquares(&lines, piece, position);
      break;
    case 'p':
      getPawnSquares(&lines, piece, position);
      break;
    case 'k':
      getKingSquares(&lines, piece, position);
      break;
    case 'q':
      getStraightSquares(&lines, piece, position);
      getDiagonalSquares(&lines, piece, position);
      break;
    case 'n':
      getKnightSquares(&lines, piece);
      break;
  }

  for (int i = 0; i < lines.len; i++) {
    mv.start = piece;
    mv.end = lines.squares[i];
    if (validityCheck(position, mv, skip_king_check)) {
      if ((tolower(getPiece(mv.start, position)) == 'p') && (mv.end.y == 0 || mv.end.y == 7)) {
        for (int j = 0; j < 4; j++) {
          struct Move pm = mv;
          if (position->player == 0) {
            pm.promotion = wpromotions[j]; 
            pm.promotes = 1;
          } else {
            pm.promotion = bpromotions[j]; 
            pm.promotes = 1;
          }
          moves[*counter] = pm;
          (*counter)++;
        }
      }
      else {
        moves[*counter] = mv;
        (*counter)++;
      }
    }
  }
  
  free(lines.squares);
}

struct Move* getMoves(struct Position* position, int* counter, int skip_king_check) {
  struct Move* mvs = (struct Move*)calloc(218, sizeof(struct Move));
  struct Square sq;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sq.x = i;
      sq.y = j;
      char piece = getPiece(sq, position);
      if (piece == 'X' || (!!isupper(piece) ? 0 : 1) != position->player) {
        continue; 
      }
      genMoves(mvs, position, sq, counter, skip_king_check);
    }
  }
  return mvs;
}

int isThreatened(struct Position* position, struct Square square) {
  
  int counter = 0;
  struct Move* moves = getMoves(position, &counter, 1);
  printMoves(moves, counter); 
  for (int i = 0; i < counter; i++) {
    if (moves[i].end.x == square.x && moves[i].end.y == square.y) {
      printf("\nThreat found by %d|%d to %d|%d\n", moves[i].start.x, moves[i].start.y, moves[i].end.x, moves[i].end.y);
      return 1; 
    }
  }
  return 0;
}
