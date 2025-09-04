#include <string.h>
#include "structs.h"
#include "zobrist.c"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define INFINITY 1000000
#define NEG_INFINITY -1000000

const char wpromotions[4] = "QRBN";
const char bpromotions[4] = "qrbn";
int mallocCounter = 0;
int callocCounter = 0;
int mcounter;
int isThreatened(struct Position position, struct Square square);

void printPosition(struct Position position);

struct Position copyPosition(const struct Position* original) {
    struct Position copy;
    memcpy(&copy, original, sizeof(struct Position));
    return copy;
}


struct Position FENtoPosition(const char* fen) {
    struct Position position;
    // Initialize board to '.'
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            position.board[i][j] = '.';
        }
    }


    int over = 0;
    const int len = strlen(fen);
    char rank = 7;  // Start at rank 8 (index 7)
    int file = 0;
    int field = 0;

    for (int i = 0; i < len; i++) {
        char nextchar = fen[i];

        if (over) {
            // Process fields after board section
            switch (field) {
                case 1:
                    position.player = (nextchar == 'w') ? 0 : 1;
                    field++;
                    break;
                // Add other field processing as needed
            }
            continue;
        }

        if (rank < 0) {
            // Finished all 8 ranks
            over = 1;
            field++;
            i--;  // Reprocess current character in field mode
            continue;
        }

        if (isalpha(nextchar)) {
            position.board[file][rank] = nextchar;
            file++;
        } else if (nextchar == '/') {
            rank--;
            file = 0;
        } else if (isdigit(nextchar)) {
            int emptySquares = nextchar - '0';
            file += emptySquares;
        } else if (nextchar == ' ') {
            over = 1;
            field++;
        }
    }

    return position;
}

struct Move notationToMove(const char* notation, size_t len) {

  struct Move move = {{-1, -1}, {-1, -1}, 0, '.'};
  if (len < 4) return move;

  move.start.x = notation[0] - 'a';
  move.start.y = notation[1] - '1';


  move.end.x = notation[2] - 'a';
  move.end.y = notation[3] - '1';

  //promotion
  if (len == 5) {
    move.promotion = notation[4];
    move.promotes = 1;
  }

  return move;
}

struct Move* notationsToMoves(const char* notation, size_t len, int *sc) {
  for (int i = 0; i <= len; i++) {
    if (notation[i] == ' ') {
      (*sc)++;
    }
  }
  mallocCounter++;
  struct Move* moves = malloc(sizeof(struct Move) * (*sc));
  for (int i = 0; i < *sc; i++) {
    while (*notation == ' ') notation++;
    size_t move_len = 0;
    while (notation[move_len] != ' ' && notation[move_len] != '\0') {
      move_len++;
    }
    moves[i] = notationToMove(notation, move_len);
    notation += move_len;
  }
  return moves;
}

char getPiece(const struct Square sq, const struct Position* pos) {
  if (sq.x >= 0 && sq.x < 8 && sq.y >= 0 && sq.y < 8) {
    return pos->board[sq.x][sq.y];
  }
  return '.';
}
int getSpecialMoveType(const struct Position* pos, const struct Move move) {
  const char piece = getPiece(move.start, pos);

  if (tolower(piece) == 'k' && abs(move.start.x - move.end.x) > 1) {
    return 1; //Castling
  }

  if (tolower(piece) == 'p' && move.start.x != move.end.x && getPiece(move.end, pos) == '.') {
    return 2; //En passant
  }
  return 0;
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

void ApplyEnpassantTarget(const char* fen, struct Position *position) {
    int spaceCount = 0;
    position->enpassant.valid = 0;

    for (int i = 0; fen[i] != '\0'; i++) {
        if (fen[i] == ' ') {
            spaceCount++;
            if (spaceCount == 3) {
                i++;
                // Feld ist \xe2\x80\x93 \xe2\x80\x93 kein EP
                if (fen[i] == '-') {
                    return;
                }
                if (fen[i] && fen[i+1] && islower((unsigned char)fen[i]) && isdigit((unsigned char)fen[i+1])) {
                    position->enpassant.square.x  = fen[i] - 'a';
                    position->enpassant.square.y  = fen[i+1] - '1';
                    position->enpassant.valid = 1;
                }
                return;
            }
        }
    }
}

void printMoves(const struct Move* moves, const int len, struct Position position) {
  for (int i = 0; i < len; i++) {
    printf("\n%c: %c%d%c%d", getPiece(moves[i].start, &position), moves[i].start.x + 'a', moves[i].start.y + 1, moves[i].end.x + 'a', moves[i].end.y + 1);
    printf("\n");
  }
}

void printSquares(const struct Square* squares, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("\n%d|%d", squares[i].x, squares[i].y);
  }
}

void promote(struct Position* position, struct Move move) {
  position->board[move.end.x][move.end.y] = move.promotion;
}

void printPosition(const struct Position position) {
  printf("\n");
  printf("Player: %s\n", position.player == 0 ? "White" : "Black");
  for (int i = 7; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      char piece = position.board[j][i];
      if (piece == 0) {
        printf("N ");
      } else {
        printf("%c ", piece);
      }
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
  callocCounter++;
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
  const int player_to_check = position->player;
  const char king_to_find = (player_to_check == 0) ? 'K' : 'k';
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

  const int threatened = isThreatened(*position, kingSquare);

  position->player = player_to_check;

  return threatened;
}

struct Square getCastleRook(const struct Position* position, struct Move move) {
    struct Square rook;
    int dir = (move.end.x > move.start.x) ? 1 : -1;
    int x = move.start.x + dir;
    int y = move.start.y;

    while (x >= 0 && x < 8) {
        if (tolower(position->board[x][y]) == 'r') {
            rook.x = x;
            rook.y = y;
//            printf("Rook %c position: %d|%d\n", getPiece(rook, position), rook.x, rook.y);
            return rook;
        }
        x += dir;
    }
    rook.x = -1;
    rook.y = -1;

    return rook;
}


int removeCastling(const struct Position* position, struct Square rook) {
  switch (position->wcastle)
    {
    case 0:
      return 0;

    case 1: // kingside
      if (rook.x == 7) {
        return 0;
      }
      break;

    case 2: // queenside
      if (rook.x == 0) {
        return 0;
      }
      break;

    case 3: // both
      if (rook.x == 7) {
        return 2;
      }
      else if (rook.x == 0) {
        return 1;
      }
      break;
    default: printf("\nerror\n");
    }

    return 0;
}

int isValidPiece(char piece) {
  return piece == 'P' || piece == 'p' ||
         piece == 'N' || piece == 'n' ||
         piece == 'B' || piece == 'b' ||
         piece == 'R' || piece == 'r' ||
         piece == 'Q' || piece == 'q' ||
         piece == 'K' || piece == 'k' ||
         piece == '.';
}

void movePiece(struct Position* position, struct Move move) {
  char piece = position->board[move.start.x][move.start.y];
  position->board[move.start.x][move.start.y] = '.';
  position->board[move.end.x][move.end.y] = piece;
}





int epValid(const struct Position* position, const struct Move move) {
  struct Square epsquare1;
  struct Square epsquare2;
  epsquare1.x = move.end.x + 1;
  epsquare1.y = move.end.y;
  epsquare2.x = move.end.x - 1;
  epsquare2.y = move.end.y;
  if (tolower(getPiece(epsquare1, position)) == 'p') {
    return 1;
  }
  if (tolower(getPiece(epsquare2, position)) == 'p') {
    return 1;
  }
  epsquare1.x = -1;
  epsquare1.y = -1;
  return 0;

}


struct moveReturn makeMove(struct Position* position, const struct Move move, const int special) { // special: 0 = normal move, 1 = castling 2 = e.p.
  struct moveReturn ret;

  ret.states[0].square = move.start;
  ret.states[0].piece = position->board[move.start.x][move.start.y];
  ret.states[1].square = move.end;
  ret.states[1].piece = position->board[move.end.x][move.end.y];
  ret.enpassant = position->enpassant;
  position->enpassant.valid = 0;

  if (special == 1) { //castling
    ret.states[2].square = getCastleRook(position, move);
    ret.states[2].piece = position->board[ret.states[2].square.x][ret.states[2].square.y];
    ret.states[3].square.x = move.start.x + ((move.end.x - move.start.x > 1) ? 1 : -1);
    ret.states[3].square.y = move.start.y;
    ret.states[3].piece = position->board[ret.states[3].square.x][ret.states[3].square.y];
    ret.len = 4;
  }
  else if (special == 2) { //en passant
    ret.states[2].square.x = move.end.x;
    ret.states[2].square.y = move.start.y;
    ret.states[2].piece = position->board[ret.states[2].square.x][ret.states[2].square.y];
    ret.len = 3;
  }

  else {
    ret.len = 2;
  }

  if (!special){
    struct Square pc;
    pc.x = move.end.x;
    pc.y = move.end.y;
    const char pChar = getPiece(pc, position);
    if (pChar == 'P' || pChar == 'p') { // check if its a pawn
      if (isupper(pChar) && move.end.y - move.start.y == 2) { // white pawn moving 2 squares
        if (epValid(position, move)) {
          position->enpassant.square.x = move.end.x;
          position->enpassant.square.y = move.end.y - 1;
          position->enpassant.valid = 1;
        }

      }

      else if (islower(pChar) && move.start.y - move.end.y == 2) {
        if (epValid(position, move)) {
          position->enpassant.square.x = move.end.x;
          position->enpassant.square.y = move.end.y + 1;
          position->enpassant.valid = 1;
        }
      }

    }
    movePiece(position, move);
  }
  else if (special == 1) {
    struct Square rook = getCastleRook(position, move);
    int dir = (move.end.x - move.start.x > 1);
    movePiece(position, move);

    struct Move mv2;
    mv2.start = rook;
    mv2.end.x = move.start.x + (dir ? 1 : -1);
    mv2.end.y = move.start.y;
    movePiece(position, mv2);
  }
  else if (special == 2) { //e.p.
    movePiece(position, move);
    position->board[move.end.x][move.start.y] = '.'; //capture pawn
}
  if (move.promotes) {
    promote(position, move);
  }
  position->player = abs(position->player - 1); //its the other players turn now
  return ret;
}

void undoMove(struct Position* position, struct moveReturn ret) {
  for (size_t i = 0; i < ret.len; i++) {
    position->board[ret.states[i].square.x][ret.states[i].square.y] = ret.states[i].piece;
  }
  position->player = abs(position->player - 1);
  position->enpassant = ret.enpassant;
}

struct Path getPath(struct Move move, int dir) {
  if (dir == 0) {
    return getPathStraight(move);
  }
  return getPathDiagonal(move);
}

int qrsvc(const struct Position* position, struct Move move, int owner) { //Queen-Rook-Straight-Validity-Check

  struct Path path = getPath(move, 0);
  for (int i = 0; i < path.len; i++) {
    if (getPiece(path.squares[i], position) != '.') {
      free(path.squares);
      return 0;
    }
  }
  free(path.squares);
  return 1;
}

int qbdvc(const struct Position* position, struct Move move, int owner) { //Queen-Bishop-diagonal-validitiy check
    struct Path path = getPath(move, 1);
    for (int i = 0; i < path.len; i++) {

    if (getPiece(path.squares[i], position) != '.') {
      free(path.squares);
      return 0;
    }
  }
  free(path.squares);
  return 1;
}


int castleCheck(struct Position position, const struct Square sq) {
  if ((getPiece(sq, &position) != '.')) {
    return 0;
    }

  position.player = abs(position.player - 1);

  if (isThreatened(position, sq)) {
    return 0;
  }

  return 1;
}

int kvc(struct Position position, const struct Move move, const int owner, const int skip_king_check, int castle) {
  const int opponent = (owner == 0) ? 1 : 0;

  const int original_player = position.player;
  position.player = opponent;
  if (skip_king_check == 0) {
    const int threatened = isThreatened(position, move.end);


    if (threatened) {
      return 0;
    }
  }
  position.player = original_player;
  if (abs(move.start.x - move.end.x) > 1 && castle == 0) { //castling
    struct Square rook = getCastleRook(&position, move);
    if (rook.x == -1) {
      return 0;
    }
    const char pc = getPiece(move.start, &position);
    if (isupper(pc)) { // white
        if (position.wcastle == 0) {
          printf("\nno castling rights for white\n");
          return 0;
        }
        if (move.end.x == 6 && move.start.x == 4 && (position.wcastle == 1 || position.wcastle == 3)) { //kingside
            for (int i = 5; i < 7; i++) {
              struct Square sq = {i, move.start.y};
              if (!castleCheck(position, sq)) {return 0;}
            }
        }
        else if (move.start.x == 4 && move.end.x == 2 && (position.wcastle == 2 || position.wcastle == 3)) { //queenside
            for (int i = 3; i > 0; i--) {
              struct Square sq = {i, move.start.y};
              if (!castleCheck(position, sq)) {return 0;}
            }
        }
    }
    else {
      //black
      if (position.bcastle == 0) {
        printf("\nno castling rights for black\n");
        return 0;
      }
      if (move.start.x == 4 && move.end.x == 6 && (position.bcastle == 1 || position.bcastle == 3)) { //kingside
        for (int i = 5; i < 7; i++) {
          struct Square sq = {i, move.start.y};
          if (!castleCheck(position, sq)) {return 0;}
        }
      }
      else if (move.start.x == 4 && move.end.x == 2 && (position.bcastle == 2 || position.bcastle == 3)) { //queenside
        for (int i = 3; i > 0; i--) {
          struct Square sq = {i, move.start.y};
          if (!castleCheck(position, sq)) {return 0;}
        }
      }
    }
  }

  return 1;

}



int pvc(const struct Position* position, const struct Move move, const int owner) {

  if (move.end.x == position->enpassant.square.x && move.end.y == position->enpassant.square.y) {
    return 1;
  }
  const int dir = (owner == 0) ? 1 : -1;

  if (move.start.x == move.end.x) {
    if (getPiece(move.end, position) != '.') {
      return 0;
    }

    if (move.end.y - move.start.y == dir) {
        return 1;
    }

    if (move.end.y - move.start.y == 2 * dir) {
        int start_rank = (owner == 0) ? 1 : 6;
        if (move.start.y == start_rank) {
            struct Move singleMove = move;
            singleMove.end.y = move.start.y + dir;
            return qrsvc(position, singleMove, owner);
        }
    }
    return 0;
  }
  if ((abs(move.start.x - move.end.x) == 1) && (move.end.y - move.start.y == dir)){ // Pawn captures diagonal
    char dpiece = getPiece(move.end, position);
    if (dpiece == '.') {
      return 0;
    }
    if ((!!isupper(dpiece) ? 0 : 1) == owner) {
      return 0;
    }
    return 1;
  }
  return 0;
}


int validityCheck(const struct Position* position, struct Move move, const int skip_king_check, int castle) {
  if (move.end.x < 0 || move.end.x > 7 || move.end.y < 0 || move.end.y > 7) {
    return 0;
  }



  const char sp = position->board[move.start.x][move.start.y];
  if (sp == '.') {
    return 0;
  }

  const int owner = !!isupper((unsigned char)sp) ? 0 : 1;
  if (owner != position->player) {
    return 0;
  }

  const char dp = position->board[move.end.x][move.end.y];
  if (dp != '.' && (!!isupper((unsigned char)dp) ? 0 : 1) == owner) {
      return 0;
  }

  const char piece_type = tolower(sp);

  switch(piece_type) {
    case 'p':
      return pvc(position, move, owner);
    case 'k':
      return kvc(*position, move, owner, skip_king_check, castle);
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
    default: printf("\nerror\n");
  }
  printf("\nreturn 0\n");
  return 0;
}



int valid(struct Position* position, struct Move move, const int skip_king_check, int castle) {
  if (validityCheck(position, move, skip_king_check, castle)) {
    if (skip_king_check == 0) {
      const int special_type = getSpecialMoveType(position, move);
      const struct moveReturn ret = makeMove(position, move, special_type);
      if (incheck(position)) {
        undoMove(position, ret);
        return 0;
      }
      undoMove(position, ret);
      return 1;
    }
      return 1;
  }
  return 0;
}



void getDiagonalSquares(struct Path* lines, struct Square start, const struct Position* position) {
    int count = lines->len;

    for (int dir = 0; dir < 4; dir++) {
      const int dy[] = {-1, 1, -1, 1};
      const int dx[] = {-1, -1, 1, 1};
      int x = start.x + dx[dir];
        int y = start.y + dy[dir];

        while (x >= 0 && x < 8 && y >= 0 && y < 8 && count < 28) {
            lines->squares[count].x = x;
            lines->squares[count].y = y;
            count++;

            struct Square current_square = {x, y};
            if (getPiece(current_square, position) != '.') {
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
            if (getPiece(current_square, position) != '.') {
                break;
            }

            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len = count;
}

int getIsolatedPaws(struct Position position) {
  int isolated = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (tolower(position.board[i][j]) == 'p') {
        int hasNeighbor = 0;
        for (int row = 0; row < 8 && !hasNeighbor; row++) {
          if ((j > 0 && tolower(position.board[j-1][row]) == 'p') ||
              (j < 7 && tolower(position.board[j+1][row]) == 'p')) {
            hasNeighbor = 1;
              }
        }
        if (!hasNeighbor) isolated++;
      }
    }
  }
  return isolated;
}

int getDoublePawns(struct Position position) {
  int score = 0;
  int count = 0;
  for (int i = 0; i < 8; i++) { //y
    for (int j = 0; j < 8; j++) { //x
      if (tolower(position.board[i][j]) == 'p') {
        count++;
      }
    }
    if (count > 1) {
      score -= 20;
    }
    else if (count > 2) {
      score -= 40;
    }
  }
  return score;
}


void getPawnSquares(struct Path* lines, struct Square piece, struct Position* position) {
  int dir = (!!isupper(getPiece(piece, position))) ? 1 : -1;
  int count = 0;

  // 1 square forward
  int x = piece.x;
  int y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    struct Square one_forward = {x, y};
    if (getPiece(one_forward, position) == '.') {
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
                if (getPiece(two_forward, position) == '.') {
                    lines->squares[count].x = x;
                    lines->squares[count].y = y;
                    count++;
                }
            }
        }
    }
  }

  x = piece.x + 1;
  y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    lines->squares[count].x = x;
    lines->squares[count].y = y;
    count++;
  }

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

void genMoves(struct Move* moves, struct Position* position, struct Square piece, int* counter, const int skip_king_check, const int castle, const int onlyCount, const int skipPawns, struct Path lines) {

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
      if (!skipPawns) {
        getPawnSquares(&lines, piece, position);
      }
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

    if (valid(position, mv, skip_king_check, castle) == 1) {

      if ((tolower(getPiece(mv.start, position)) == 'p') && (mv.end.y == 0 || mv.end.y == 7)) { //promotion
        for (int j = 0; j < 4; j++) {
          struct Move pm = mv;
          if (position->player == 0) {
            pm.promotion = wpromotions[j];
            pm.promotes = 1;
          } else {
            pm.promotion = bpromotions[j];
            pm.promotes = 1;
          }
          if (!onlyCount) {
          moves[*counter] = pm;
          }
          (*counter)++;
        }
      }
      else {
        mv.promotion = 0;
        mv.promotes = 0;
        if (!onlyCount) {
          moves[*counter] = mv;
        }
        (*counter)++;
      }
    }
  }

}

inline int getPieceValue(const char piece) {
  switch (piece | 32) {
    case 'p': return 100;
    case 'n': return 320;
    case 'b': return 330;
    case 'r': return 500;
    case 'q': return 900;
    case 'k': return 10000;
    default: return 0;
  }
}

int quickMoveScore(const struct Position* position, struct Move move) {
  int score = 0;

  char victim = getPiece(move.end, position);
  char attacker = getPiece(move.start, position);

  if (victim != '.') {
    int victim_value = getPieceValue(victim);
    int attacker_value = getPieceValue(attacker);
    score += victim_value * 10 - attacker_value / 10;

    if (victim_value >= attacker_value) {
      score += 1000;
    }
  }

  if (move.promotes) {
    score += getPieceValue(move.promotion) + 800;
  }


  char piece = tolower(getPiece(move.start, position));
  if (piece == 'q' || piece == 'r' || piece == 'b') {
    score += 50;
  }

  if ((move.end.x >= 2 && move.end.x <= 5) &&
      (move.end.y >= 2 && move.end.y <= 5)) {
    score += 30;
      }

  if ((piece == 'n' || piece == 'b') &&
      ((position->player == 0 && move.start.y == 0) ||
       (position->player == 1 && move.start.y == 7))) {
    score += 20;
       }

  return score;
}


void getMoves(struct Position* position, int* counter, const int skip_king_check, const int castle, const int onlyCount, int skipPawns, struct Move* mvs) {

  struct Path lines;
  mallocCounter++;
  lines.squares = malloc(sizeof(struct Square) * 28);
  struct Square sq;

  *counter = 0;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sq.x = i;
      sq.y = j;
      char piece = getPiece(sq, position);
      if (piece == '.' || (!!isupper(piece) ? 0 : 1) != position->player) {
        continue;
      }
      lines.len = 0;
      genMoves(mvs, position, sq, counter, skip_king_check, castle, onlyCount, skipPawns, lines);
    }
  }

  free(lines.squares);

  if (!onlyCount && *counter > 1) {
    for (int i = 1; i < *counter; i++) {
      struct Move key = mvs[i];
      int keyScore = quickMoveScore(position, key);
      int j = i - 1;

      while (j >= 0 && quickMoveScore(position, mvs[j]) < keyScore) {
        mvs[j + 1] = mvs[j];
        j--;
      }
      mvs[j + 1] = key;
    }
  }
}


void getMoves_withOrdering(struct Position* position, struct Move* moves, int* counter) {
    getMoves(position, counter, 0, 0, 0, 0, moves);


    int scores[218];
    for (int i = 0; i < *counter; i++) {
        scores[i] = 0;

        char victim = getPiece(moves[i].end, position);
        char attacker = getPiece(moves[i].start, position);

        // Captures (wichtigste Verbesserung!)
        if (victim != '.') {
            int victim_val = getPieceValue(victim);
            int attacker_val = getPieceValue(attacker);

            // MVV-LVA: Wertvollstes Opfer zuerst, billigster Angreifer zuerst
            scores[i] += victim_val * 100 + (1000 - attacker_val);

            // Extra Bonus für gewinnende Captures
            if (victim_val >= attacker_val) {
                scores[i] += 10000;
            }
        }

        // Promotions
        if (moves[i].promotes) {
            scores[i] += getPieceValue(moves[i].promotion) * 10 + 5000;
        }

        // Castling
        if (getSpecialMoveType(position, moves[i]) == 1) {
            scores[i] += 100;
        }

        int center_dist = abs(moves[i].end.x - 3.5) + abs(moves[i].end.y - 3.5);
        scores[i] += (7 - center_dist) * 5;
    }

    for (int i = 0; i < *counter - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < *counter; j++) {
            if (scores[j] > scores[max_idx]) {
                max_idx = j;
            }
        }
        if (max_idx != i) {
            struct Move temp_move = moves[i];
            moves[i] = moves[max_idx];
            moves[max_idx] = temp_move;

            int temp_score = scores[i];
            scores[i] = scores[max_idx];
            scores[max_idx] = temp_score;
        }
    }
}

int isThreatened(struct Position position, struct Square target) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      char pc = position.board[i][j];
      if (pc == '.') {
        continue;
      }
      else if (isupper(pc) == position.player) {
        struct Move mv;
        mv.start.x = i;
        mv.start.y = j;
        mv.end = target;
        if (validityCheck(&position, mv, 0, 0)) {
          return 1;
        }
      }
    }
  }
  return 0;
}

int getMaterialValue(const struct Position position) {
  int value = 0;
  for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            char piece = position.board[i][j];
            if (piece == '.') continue;
            switch (piece) {
                case 'p': value -= 100; break;
                case 'n': value -= 320; break;
                case 'b': value -= 330; break;
                case 'r': value -= 500; break;
                case 'q': value -= 900; break;
                case 'k': value -= 10000; break;
                case 'P': value += 100; break;
                case 'N': value += 320; break;
                case 'B': value += 330; break;
                case 'R': value += 500; break;
                case 'Q': value += 900; break;
                case 'K': value += 10000; break;
              default:;
            }
        }
    }
    return value;
}

int getCenterValue(struct Position position) {
  int score = 0;
  const int center_squares[4][2] = {{3, 3}, {3, 4}, {4, 3}, {4, 4}};

  for (int i = 0; i < 4; i++) {
    struct Square sq = {center_squares[i][0], center_squares[i][1]};
    char piece = getPiece(sq, &position);
    if (piece != '.') {
      if ((!!isupper(piece) ? 0 : 1) == position.player) {
        score += 10; // Bonus for occupying the center
      }
    }
  }
  return score;
}

int getMobility(struct Position position);

int getValue(const struct Position position) {
  int v = 0;
  v += getMobility(position);
  v += getMaterialValue(position);
  v += getCenterValue(position);

  struct Position newposition = position;
  newposition.player = abs(newposition.player - 1);

  v -= getCenterValue(newposition);
  v -= getMobility(newposition);
  v += getDoublePawns(position);
  return v;
}
int negamax(struct Position* pos, int depth, int alpha, int beta) {
  if (depth == 0) {
    int eval = getValue(*pos);
    return pos->player == 0 ? eval : -eval;
  }

  int counter = 0;
  struct Move moves[218];

  getMoves_withOrdering(pos, moves, &counter);

  int best = -INFINITY;

  for (int i = 0; i < counter; i++) {
    struct Position new_pos = copyPosition(pos);
    makeMove(&new_pos, moves[i], getSpecialMoveType(pos, moves[i]));

    int eval = -negamax(&new_pos, depth - 1, -beta, -alpha);

    if (eval > best) {
      best = eval;
    }

    if (eval > alpha) {
      alpha = eval;
    }

    if (alpha >= beta) {
      break;
    }
  }

  return best;
}

struct Move getBestMoveNegamax(struct Position* pos, int depth) {
    int alpha = -INFINITY;
    int beta = INFINITY;
    int best_eval = -INFINITY;
    struct Move best_move = {{-1, -1}, {-1, -1}, 0, '.'};

    int counter = 0;
    struct Move moves[218];
    getMoves(pos, &counter, 0, 0, 0, 0, moves);

    fflush(stdout);

    for (int i = 0; i < counter; i++) {
        struct Position new_pos = copyPosition(pos);
        makeMove(&new_pos, moves[i], getSpecialMoveType(pos, moves[i]));

        int eval = -negamax(&new_pos, depth - 1, -beta, -alpha);


        if (eval > best_eval) {
            best_eval = eval;
            best_move = moves[i];
        }

        if (eval > alpha) {
            alpha = eval;
        }
    }


    fflush(stdout);

    return best_move;
}

int getNumberOfMoves(struct Position pos, const int skipPawns) {
  int counter = 0;
  struct Move* mvs = 0;
  getMoves(&pos, &counter, 0, 0, 1, skipPawns, mvs);
  return counter;
}

int getMobility(struct Position position) {
  return getNumberOfMoves(position, 1) * 10;
}
