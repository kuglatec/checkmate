#include <string.h>
#include "structs.h"
#include "tables.c"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#define INFINITY 1000000
#define NEG_INFINITY -1000000
#define HASH_EXACT       0
#define HASH_LOWER_BOUND 1  //alphacuttof
#define HASH_UPPER_BOUND 2  //beta cutoff
#define MAX_PIECES 17
//test
const char wpromotions[4] = "QRBN";
const char bpromotions[4] = "qrbn";

static struct Square path_buffer[28];

int isThreatened(struct Position position, struct Square square);

void printPosition(struct Position position);
int quiescence_search(struct Position* pos, int alpha, int beta, struct HashTable* hash_table, uint64_t zobrist_key);
void getCaptureMoves(struct Position* position, struct Move* moves, int* counter);

struct Position copyPosition(const struct Position* original) {
    struct Position copy;
    memcpy(&copy, original, sizeof(struct Position));
    return copy;
}

struct Position FENtoPosition(const char* fen) {
    struct Position position;
    position.bcastled = 0;
    position.wcastled = 0;
    position.player = 0;
    position.bcastle = 0;
    position.wcastle = 0;

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
                  default: ;
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
  static struct Move moves[256];
  *sc = 0;
  const char* p = notation;

  while (p < notation + len && *p) {
    // Skip leading whitespace
    while (*p == ' ' || *p == '\n') {
      p++;
    }
    if (*p == '\0') break;

    // Find the end of the move
    const char* end = p;
    while (*end && *end != ' ' && *end != '\n') {
      end++;
    }

    if (end > p) {
      moves[*sc] = notationToMove(p, end - p);
      (*sc)++;
      if (*sc >= 256) break; // Avoid buffer overflow
    }
    p = end;
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
          default:
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

void printPosition(struct Position position) {
  printf("\n");
  printf("Player: %s\n", position.player == 0 ? "White" : "Black");
  for (int i = 7; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      char piece = position.board[j][i];
      if (piece == '.') {
        printf(". ");
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

  for (int i = 0; i < len; i++) {
    addSquareVectors(&sq, direction);
    path_buffer[i] = sq;
  }

  struct Path pth;
  pth.len = len;
  pth.squares = path_buffer;
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

  for (int i = 0; i < len; i++) {
    addSquareVectors(&sq, direction);
    path_buffer[i] = sq;
  }

  struct Path pth;
  pth.squares = path_buffer;
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
  if (tolower(piece) == 'k')
  {
    if (position->player == 0)
    {
      position->wcastle = 0;
    }
    else
    {
      position->bcastle = 0;
    }
  }
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
    ret.wcastled = position->wcastled;
    ret.bcastled = position->bcastled;
    if (position->player == 0) {
      position->wcastled = 1;
      position->wcastle = 0;
    }
    else if (position->player == 1) {
      position->bcastled = 1;
      position->bcastle = 0;
    }
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
  position->wcastled = ret.wcastled;
  position->bcastled = ret.bcastled;
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
      return 0;
    }
  }
  return 1;
}

int qbdvc(const struct Position* position, struct Move move, int owner) { //Queen-Bishop-diagonal-validitiy check
    struct Path path = getPath(move, 1);
    for (int i = 0; i < path.len; i++) {

    if (getPiece(path.squares[i], position) != '.') {
      return 0;
    }
  }
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

struct KillerTable* createKillerTable() {
  static struct KillerTable table;

  for (int ply = 0; ply < 64; ply++) {
    for (int slot = 0; slot < 2; slot++) {
      table.killers[ply][slot].start.x = -1;
      table.killers[ply][slot].start.y = -1;
      table.killers[ply][slot].end.x = -1;
      table.killers[ply][slot].end.y = -1;
      table.killers[ply][slot].promotes = 0;
      table.killers[ply][slot].promotion = '.';
    }
  }
  return &table;
}

int moves_equal(struct Move a, struct Move b) {
  return (a.start.x == b.start.x && a.start.y == b.start.y &&
          a.end.x == b.end.x && a.end.y == b.end.y &&
          a.promotes == b.promotes && a.promotion == b.promotion);
}

int isValidKiller(struct Move move) {
  return (move.start.x != -1 && move.start.y != -1);

}

int isQuietMove(const struct Position* pos, struct Move move) {
  return (getPiece(move.end, pos) == '.' && !move.promotes);
}



void updateKillers(struct KillerTable* killers, int ply, struct Move move, const struct Position* pos) {
  if (!isQuietMove(pos, move)) {
    return;
  }

  if (ply >= 64 || ply < 0) {
    return;
  }

  if (moves_equal(killers->killers[ply][0], move)) {
    return;
  }

  killers->killers[ply][1] = killers->killers[ply][0];
  killers->killers[ply][0] = move;
}

int valid(struct Position* position, struct Move move, const int skip_king_check, int castle) {
  if (validityCheck(position, move, skip_king_check, castle)) {
    if (skip_king_check == 0) {
      const int original_player = position->player;
      const int special_type = getSpecialMoveType(position, move);
      const struct moveReturn ret = makeMove(position, move, special_type);

      position->player = original_player;

      if (incheck(position)) {
        position->player = abs(original_player - 1);
        undoMove(position, ret);
        return 0;
      }

      position->player = abs(original_player - 1);
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

  for (int file = 0; file < 8; file++) {
    int white_pawns = 0;
    int black_pawns = 0;

    for (int rank = 0; rank < 8; rank++) {
      char piece = position.board[file][rank];
      if (piece == 'P') white_pawns++;
      else if (piece == 'p') black_pawns++;
    }

    // Bestrafung für Doppelbauern
    if (white_pawns > 1) {
      score -= 20 * (white_pawns - 1);  // -20 pro zusätzlichem Bauern
    }
    if (black_pawns > 1) {
      score += 20 * (black_pawns - 1);  // +20 wenn Schwarz Doppelbauern hat
    }
  }

  // Aus Sicht des aktuellen Spielers
  if (position.player == 1) {
    score = -score;
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
    default: printf("\ninvalid move: %c / %d|%d\n", piece_type, piece.x, piece.y);
  }

  for (int i = 0; i < lines.len; i++) {
    mv.start = piece;
    mv.end = lines.squares[i];
    mv.promotes = 0;
    mv.promotion = 0;
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

extern inline int getPieceValue(const char piece) {
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
  struct Square squares[28];
  struct Path lines;
  lines.len = 0;
  lines.squares = squares;
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
void getMoves_withOrdering(struct Position* position, struct Move* moves, int* counter, struct KillerTable* killers, int ply) {
    getMoves(position, counter, 0, 0, 0, 0, moves);

    int scores[218];
    for (int i = 0; i < *counter; i++) {
        scores[i] = 0;

        char victim = getPiece(moves[i].end, position);
        char attacker = getPiece(moves[i].start, position);

        if (victim != '.') {
            int victim_val = getPieceValue(victim);
            int attacker_val = getPieceValue(attacker);

            scores[i] += victim_val * 100 + (1000 - attacker_val);

            if (victim_val >= attacker_val) {
                scores[i] += 10000;
            }
        }

        if (moves[i].promotes) {
            scores[i] += getPieceValue(moves[i].promotion) * 10 + 5000;
        }

        if (ply >= 0 && ply < 64) {
            if (moves_equal(moves[i], killers->killers[ply][0])) {
                scores[i] += 2000; // Erster Killer
            } else if (moves_equal(moves[i], killers->killers[ply][1])) {
                scores[i] += 1000; // Zweiter Killer
            }
        }

        if (getSpecialMoveType(position, moves[i]) == 1) {
            scores[i] += 100;
        }

        int center_dist = abs(moves[i].end.x - 3.5) + abs(moves[i].end.y - 3.5);
        scores[i] += (7 - center_dist) * 5;
    }

//sort

  for (int i = 0; i < *counter - 1; i++) {
        int max_idx = i;
        for (int j = i + 1; j < *counter; j++) {
            if (scores[j] > scores[max_idx]) {
                max_idx = j;
            }
          
        }
        if (max_idx != i) {
            // Tausche Moves
            struct Move temp_move = moves[i];
            moves[i] = moves[max_idx];
            moves[max_idx] = temp_move;

            // Tausche Scores
            int temp_score = scores[i];
            scores[i] = scores[max_idx];
            scores[max_idx] = temp_score;
        }
    }
}


static const int knight_offsets[8][2] = {
  {1,2}, {1,-2}, {-1,2}, {-1,-2}, {2,1}, {2,-1}, {-2,1}, {-2,-1}
};
// Pre-computed king move offsets
static const int king_offsets[8][2] = {
    {-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}
};

int isThreatened(struct Position position, struct Square target) {
    const int opponent = position.player;
    const char enemy_pieces[6] = {
        (opponent == 0) ? 'P' : 'p', // pawn
        (opponent == 0) ? 'N' : 'n', // knight
        (opponent == 0) ? 'B' : 'b', // bishop
        (opponent == 0) ? 'R' : 'r', // rook
        (opponent == 0) ? 'Q' : 'q', // queen
        (opponent == 0) ? 'K' : 'k'  // king
    };

    // Pawn attacks
    const int pawn_dir = (opponent == 0) ? 1 : -1;
    const int pawn_rank = target.y - pawn_dir;
    if (pawn_rank >= 0 && pawn_rank < 8) {
        if (target.x > 0 && position.board[target.x - 1][pawn_rank] == enemy_pieces[0]) return 1;
        if (target.x < 7 && position.board[target.x + 1][pawn_rank] == enemy_pieces[0]) return 1;
    }

    // Knight attacks
    for (int i = 0; i < 8; i++) {
        const int x = target.x + knight_offsets[i][0];
        const int y = target.y + knight_offsets[i][1];
        if (x >= 0 && x < 8 && y >= 0 && y < 8) {
            if (position.board[x][y] == enemy_pieces[1]) return 1;
        }
    }

    // King attacks
    for (int i = 0; i < 8; i++) {
        const int x = target.x + king_offsets[i][0];
        const int y = target.y + king_offsets[i][1];
        if (x >= 0 && x < 8 && y >= 0 && y < 8) {
            if (position.board[x][y] == enemy_pieces[5]) return 1;
        }
    }

    // Sliding piece attacks (optimized ray casting)
    // Horizontal/Vertical (Rook/Queen)
    static const int straight_dirs[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}};
    for (int d = 0; d < 4; d++) {
        const int dx = straight_dirs[d][0];
        const int dy = straight_dirs[d][1];

        for (int x = target.x + dx, y = target.y + dy;
             x >= 0 && x < 8 && y >= 0 && y < 8;
             x += dx, y += dy) {
            const char piece = position.board[x][y];
            if (piece != '.') {
                if (piece == enemy_pieces[3] || piece == enemy_pieces[4]) return 1;
                break;
            }
        }
    }

    // Diagonal (Bishop/Queen)
    static const int diag_dirs[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (int d = 0; d < 4; d++) {
        const int dx = diag_dirs[d][0];
        const int dy = diag_dirs[d][1];

        for (int x = target.x + dx, y = target.y + dy;
             x >= 0 && x < 8 && y >= 0 && y < 8;
             x += dx, y += dy) {
            const char piece = position.board[x][y];
            if (piece != '.') {
                if (piece == enemy_pieces[2] || piece == enemy_pieces[4]) return 1;
                break;
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

int getPositionalValue(const struct Position position) {
    int score = 0;
    int total_pieces = 0;
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            char piece = position.board[f][r];
            if (piece == '.') continue;
            total_pieces++;

            int rank_for_eval = isupper(piece) ? r : 7 - r;
            int file_for_eval = f;

            switch (tolower(piece)) {
                case 'p':
                    score += (isupper(piece) ? 1 : -1) * pawn_pst[rank_for_eval][file_for_eval];
                    break;
                case 'n':
                    score += (isupper(piece) ? 1 : -1) * knight_pst[rank_for_eval][file_for_eval];
                    break;
                case 'b':
                    score += (isupper(piece) ? 1 : -1) * bishop_pst[rank_for_eval][file_for_eval];
                    break;
                case 'r':
                    score += (isupper(piece) ? 1 : -1) * rook_pst[rank_for_eval][file_for_eval];
                    break;
                case 'q':
                    score += (isupper(piece) ? 1 : -1) * queen_pst[rank_for_eval][file_for_eval];
                    break;
                case 'k':
                    if (total_pieces > 10) { //early / midgmae
                        score += (isupper(piece) ? 1 : -1) * king_pst_early[rank_for_eval][file_for_eval];
                    } else { //lategame
                        score += (isupper(piece) ? 1 : -1) * king_pst_late[rank_for_eval][file_for_eval];
                    }
                    break;
            }
        }
    }
    return score;
}

void subPiece(int* value, char piece)
{
  switch (toupper(piece))
  {
  case 'K':
    (*value) += 10;

  case 'N':
  case 'B':
    (*value) -= 10;

  default: ;
  }
}

int getDevelopment(const struct Position position) //penalizes undeveloped pieces
{
  int pl = 0;
  int value = 0;
  if (position.player != 0)
  {
    pl = 7;
  }

  for (int i = 0; i < 8; i++)
  {
    subPiece(&value, position.board[i][pl]);
  }
  return value;
}

int getValue(const struct Position position) {
  int v = 0;
  v += getPositionalValue(position);
  v += getDevelopment(position);
  v += getMobility(position);
  v += getMaterialValue(position);
  v += getCenterValue(position);

  struct Position newposition = position;
  newposition.player = abs(newposition.player - 1);

  v -= getCenterValue(newposition);
  v -= getMobility(newposition);
  v += getDoublePawns(position);

  int castling_bonus = 0;

  if (position.wcastled) {
    castling_bonus += 50;
  } else if (position.wcastle > 0) {
    castling_bonus += 20;
  } else {
    castling_bonus -= 30;
  }


  if (position.bcastled) {
    castling_bonus -= 50;
  } else if (position.bcastle > 0) {
    castling_bonus -= 20;
  } else {
    castling_bonus += 30;
  }

  if (position.player == 1) {
    castling_bonus = -castling_bonus;
  }

  v += castling_bonus;

  return v;
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


static uint64_t piece_keys[8][8][12];  // [file][rank][piece_type]
static uint64_t castle_keys[16];       //castling
static uint64_t enpassant_keys[8];     //ep
static uint64_t player_key;            //player
static int zobrist_initialized = 0;

int piece_to_index(char piece) {
    switch (piece) {
    case 'P': return 0;
    case 'N': return 1;
    case 'B': return 2;
    case 'R': return 3;
    case 'Q': return 4;
    case 'K': return 5;
    case 'p': return 6;
    case 'n': return 7;
    case 'b': return 8;
    case 'r': return 9;
    case 'q': return 10;
    case 'k': return 11;
    default: return -1;
    }
}


void init_zobrist() {
    if (zobrist_initialized) return;

    srand((unsigned int)time(NULL));


    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            for (int piece = 0; piece < 12; piece++) {
                piece_keys[file][rank][piece] =
                    ((uint64_t)rand() << 32) | (uint64_t)rand();
            }
        }
    }

    for (int i = 0; i < 16; i++) {
        castle_keys[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }

    for (int file = 0; file < 8; file++) {
        enpassant_keys[file] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }

    player_key = ((uint64_t)rand() << 32) | (uint64_t)rand();

    zobrist_initialized = 1;
}


uint64_t hash_position(const struct Position* pos) {
    if (!zobrist_initialized) {
        init_zobrist();
    }

    uint64_t hash = 0;

//board
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            char piece = pos->board[file][rank];
            if (piece != '.') {
                int piece_index = piece_to_index(piece);
                if (piece_index >= 0) {
                    hash ^= piece_keys[file][rank][piece_index];
                }
            }
        }
    }

//castling
    int castle_state = (pos->wcastle & 3) | ((pos->bcastle & 3) << 2);
    hash ^= castle_keys[castle_state];

//ep
    if (pos->enpassant.valid &&
        pos->enpassant.square.x >= 0 && pos->enpassant.square.x < 8) {
        hash ^= enpassant_keys[pos->enpassant.square.x];
        }

    //player
    if (pos->player == 1) {
        hash ^= player_key;
    }

    return hash;
}

//Zobrist Hashing functions



uint64_t updateHashSimple(uint64_t oldHash, const struct Position* oldPos, struct Move move) {
   uint64_t hash = oldHash;

   char piece = oldPos->board[move.start.x][move.start.y];
   const int i = piece_to_index(piece);
   if (i >= 0) {
       hash ^= piece_keys[move.start.x][move.start.y][i];
       if (move.promotes) {
           hash ^= piece_keys[move.end.x][move.end.y][piece_to_index(move.promotion)];
       } else {
           hash ^= piece_keys[move.end.x][move.end.y][i];
       }
   }

   char captured = oldPos->board[move.end.x][move.end.y];
   const int c = piece_to_index(captured);
   if (c >= 0) {
       hash ^= piece_keys[move.end.x][move.end.y][c];
   }

   hash ^= player_key;
   return hash;
}

uint64_t updateHashCastling(uint64_t hash, const struct Position* oldPos, struct Move move) {
   struct Square rook = getCastleRook(oldPos, move);
   const int r = piece_to_index(oldPos->board[rook.x][rook.y]);
   if (r >= 0) {
       hash ^= piece_keys[rook.x][rook.y][r];
       int rookEndX = move.start.x + ((move.end.x > move.start.x) ? 1 : -1);
       hash ^= piece_keys[rookEndX][move.start.y][r];
   }
   return hash;
}

uint64_t updateHashEnpassant(uint64_t hash, const struct Position* oldPos, struct Move move) {
   char epPawn = oldPos->board[move.end.x][move.start.y];
   const int e = piece_to_index(epPawn);
   if (e >= 0) {
       hash ^= piece_keys[move.end.x][move.start.y][e];
   }
   return hash;
}

uint64_t updateHashCastleRights(uint64_t hash, const struct Position* oldPos, const struct Position* newPos) {
   int oldCastle = (oldPos->wcastle & 3) | ((oldPos->bcastle & 3) << 2);
   int newCastle = (newPos->wcastle & 3) | ((newPos->bcastle & 3) << 2);
   if (oldCastle != newCastle) {
       hash ^= castle_keys[oldCastle];
       hash ^= castle_keys[newCastle];
   }
   return hash;
}

uint64_t updateHashEnpassantSquare(uint64_t hash, const struct Position* oldPos, const struct Position* newPos) {
   if (oldPos->enpassant.valid && oldPos->enpassant.square.x >= 0 && oldPos->enpassant.square.x < 8) {
       hash ^= enpassant_keys[oldPos->enpassant.square.x];
   }
   if (newPos->enpassant.valid && newPos->enpassant.square.x >= 0 && newPos->enpassant.square.x < 8) {
       hash ^= enpassant_keys[newPos->enpassant.square.x];
   }
   return hash;
}

uint64_t updateHash(uint64_t oldhash, const struct Position* oldPos, struct Move move)
{
  uint64_t newhash = oldhash;
  const int special = getSpecialMoveType(oldPos, move);

  switch (special)
  {
  case 1: //castling
    newhash = updateHashSimple(newhash, oldPos, move);
    newhash = updateHashCastling(newhash, oldPos, move);
    break;

  case 2: //ep
    newhash = updateHashSimple(newhash, oldPos, move);
    newhash = updateHashEnpassant(newhash, oldPos, move);
    break;

  case 0: //normal move
  default:
      newhash = updateHashSimple(newhash, oldPos, move);
    break;
  }

  struct Position newPos = *oldPos;
  makeMove(&newPos, move, special);
  newhash = updateHashCastleRights(newhash, oldPos, &newPos);

  newhash = updateHashEnpassantSquare(newhash, oldPos, &newPos);

  return newhash;
}

struct HashTable* createHashTable(size_t size)
{
  struct HashTable* table = malloc(sizeof(struct HashTable));

  size_t entry_size = sizeof(struct HashEntry);
  size_t total_bytes = size * 1024 * 1024;
  //table->len = total_bytes / entry_size;

  table->len = 1;
  while (table->len * 2 <= total_bytes / entry_size)
  {
    table->len *= 2;
  }
  table->entries = calloc(table->len, sizeof(struct HashEntry));
  table->current_age = 0;
  return table;
}

void freeHashTable(struct HashTable* table)
{
  free(table->entries);
  free(table);
}

void clearHashTable(struct HashTable* table)
{
  memset(table->entries, 0, table->len * sizeof(struct HashEntry));
  table->current_age = 0;
}

static inline size_t getHashIndex(const struct HashTable* table, uint64_t zobrist_key) {
  return zobrist_key & (table->len - 1);
}
void logHash(struct HashTable* table, uint64_t zobrist_key, int depth, int score, int type, struct Move bestmove) {
  size_t index = getHashIndex(table, zobrist_key);
  struct HashEntry* entry = &table->entries[index];

  int should_replace = 0;

  if (entry->hash == 0) {
    should_replace = 1;  //empty
  } else if (entry->hash == zobrist_key) {
    should_replace = 1;  //same position
  } else if (depth >= entry->depth + 4) {
    should_replace = 1;  //much deeper
  } else if ((table->current_age - entry->age >= 4) && (depth >= entry->depth - 1)) {
    should_replace = 1;  //old + similar depth
  }

  if (should_replace) {
    entry->hash = zobrist_key;
    entry->depth = depth;
    entry->score = score;
    entry->type = type;
    entry->bestmove = bestmove;
    entry->age = table->current_age;
  }
}

int probeHash(const struct HashTable* table, uint64_t hash, int depth, int alpha, int beta, int* score, struct Move* bestmove) {
  size_t index = getHashIndex(table, hash);
  const struct HashEntry* entry = &table->entries[index];

  if (entry->hash != hash) {
    return 0;  //no match
  }

  if (bestmove && (entry->bestmove.start.x != -1 || entry->bestmove.start.y != -1)) {
    *bestmove = entry->bestmove;
  }

  if (entry->depth < depth) {
    return 0;
  }

  *score = entry->score;

  switch (entry->type) {
  case HASH_EXACT:
    return 1;  //exact score
  case HASH_LOWER_BOUND:
    return entry->score >= beta ? 1 : 0;  //beta cutoff
  case HASH_UPPER_BOUND:
    return entry->score <= alpha ? 1 : 0;  //alpha cutoff
  default: ;
  }

  return 0;
}

void incrementHashAge(struct HashTable* table) {
    table->current_age++;
}



void getPieceString(const struct Position pos, char *buf, int *len) {
  *len = 0; // reset length at start

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      char c = pos.board[i][j];
      if (c == 0) continue;

      if ((pos.player && islower((unsigned char)c)) ||
          (!pos.player && isupper((unsigned char)c))) {
        buf[*len] = c;
        (*len)++;
        if (*len >= MAX_PIECES - 1)
          goto done;
          }
    }
  }

  done:
      buf[*len] = '\0';
}

//detection for positions that likely contain zugzwang
int hasZugzwang(const struct Position pos) {

  int piece_count = 0;
  int has_non_pawn_pieces = 0;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      char piece = pos.board[i][j];
      if (piece == '.') continue;

      if ((pos.player == 0 && isupper(piece)) ||
          (pos.player == 1 && islower(piece))) {
        piece_count++;
        if (tolower(piece) != 'k' && tolower(piece) != 'p') {
          has_non_pawn_pieces = 1;
        }
          }
    }
  }

  return !has_non_pawn_pieces || piece_count <= 3;
}

void getCaptureMoves(struct Position* position, struct Move* moves, int* counter) {
    struct Move all_moves[218];
    int all_counter = 0;
    getMoves(position, &all_counter, 0, 0, 0, 0, all_moves);

    *counter = 0;
    for (int i = 0; i < all_counter; i++) {
        if (!isQuietMove(position, all_moves[i])) {
            moves[*counter] = all_moves[i];
            (*counter)++;
        }
    }
}

int quiescence_search(struct Position* pos, int alpha, int beta, struct HashTable* hash_table, uint64_t zobrist_key) {
    int stand_pat = getValue(*pos);
    if (pos->player == 1) stand_pat = -stand_pat;


    if (stand_pat >= beta) {
        return beta;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    struct Move moves[218];
    int counter = 0;
    getCaptureMoves(pos, moves, &counter);

    for (int i = 0; i < counter; i++) {
        struct Position new_pos = copyPosition(pos);
        uint64_t new_zobrist = updateHash(zobrist_key, pos, moves[i]);
        makeMove(&new_pos, moves[i], getSpecialMoveType(pos, moves[i]));

        int score = -quiescence_search(&new_pos, -beta, -alpha, hash_table, new_zobrist);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}




int negamaxWithKillersAndHash(struct Position* pos, int depth, int alpha, int beta,
                              struct HashTable* hash_table, uint64_t zobrist_key,
                              struct KillerTable* killers, int ply, int null_move_allowed) {

    // Hash Table Probe
    int hash_score;
    struct Move hash_move = {{-1, -1}, {-1, -1}, 0, '.'};

    if (probeHash(hash_table, zobrist_key, depth, alpha, beta, &hash_score, &hash_move)) {
        return hash_score;
    }

    if (depth == 0) {
        return quiescence_search(pos, alpha, beta, hash_table, zobrist_key);
    }

    // Null Move Pruning
    if (null_move_allowed && depth >= 3 && !incheck(pos) && !hasZugzwang(*pos)) {
        struct Position null_pos = copyPosition(pos);
        null_pos.player = abs(null_pos.player - 1);
        null_pos.enpassant.valid = 0;

        uint64_t null_zobrist = zobrist_key ^ player_key;
        if (pos->enpassant.valid && pos->enpassant.square.x >= 0 && pos->enpassant.square.x < 8) {
            null_zobrist ^= enpassant_keys[pos->enpassant.square.x];
        }

        int R = (depth > 6) ? 3 : 2;
        int null_score = -negamaxWithKillersAndHash(&null_pos, depth - 1 - R, -beta, -beta + 1,
                                                    hash_table, null_zobrist, killers, ply + 1, 0);

        if (null_score >= beta) {
            if (depth > 6) {
                int verification_score = -negamaxWithKillersAndHash(&null_pos, depth - 1 - R - 2, -beta, -beta + 1,
                                                                   hash_table, null_zobrist, killers, ply + 1, 0);
                if (verification_score >= beta) {
                    return beta;
                }
            } else {
                return beta;
            }
        }
    }

    int counter = 0;
    struct Move moves[218];
    getMoves_withOrdering(pos, moves, &counter, killers, ply);

    if (hash_move.start.x != -1 || hash_move.start.y != -1) {
        for (int i = 0; i < counter; i++) {
            if (moves[i].start.x == hash_move.start.x &&
                moves[i].start.y == hash_move.start.y &&
                moves[i].end.x == hash_move.end.x &&
                moves[i].end.y == hash_move.end.y) {
                struct Move temp = moves[0];
                moves[0] = moves[i];
                moves[i] = temp;
                break;
            }
        }
    }

    int best_score = -INFINITY;
    struct Move best_move = {{-1, -1}, {-1, -1}, 0, '.'};
    int hash_flag = HASH_UPPER_BOUND;

    for (int i = 0; i < counter; i++) {
        struct Position new_pos = copyPosition(pos);
        uint64_t new_zobrist = updateHash(zobrist_key, pos, moves[i]);
        makeMove(&new_pos, moves[i], getSpecialMoveType(pos, moves[i]));

        int score = -negamaxWithKillersAndHash(&new_pos, depth - 1, -beta, -alpha,
                                              hash_table, new_zobrist, killers, ply + 1, 1);

        if (score > best_score) {
            best_score = score;
            best_move = moves[i];
        }

        if (score > alpha) {
            alpha = score;
            hash_flag = HASH_EXACT;
        }

        // Beta Cutoff - hier werden Killer Moves aktualisiert
        if (alpha >= beta) {
            hash_flag = HASH_LOWER_BOUND;
            // Update Killers nur für quiet moves (keine Captures/Promotions)
            updateKillers(killers, ply, moves[i], pos);
            break;
        }
    }

    logHash(hash_table, zobrist_key, depth, best_score, hash_flag, best_move);
    return best_score;
}

struct Move getBestMoveWithKillers(struct Position* pos, int depth, struct HashTable* hash_table) {
  incrementHashAge(hash_table);

  //allocate new killer table
  struct KillerTable* killers = createKillerTable();

  uint64_t zobrist_key = hash_position(pos);

  int alpha = -INFINITY;
  int beta = INFINITY;
  int best_eval = -INFINITY;
  struct Move best_move = {{-1, -1}, {-1, -1}, 0, '.'};

  int counter = 0;
  struct Move moves[218];
  getMoves(pos, &counter, 0, 0, 0, 0, moves);

  for (int i = 0; i < counter; i++) {
    struct Position new_pos = copyPosition(pos);
    uint64_t new_zobrist = updateHash(zobrist_key, pos, moves[i]);
    makeMove(&new_pos, moves[i], getSpecialMoveType(pos, moves[i]));

    int eval = -negamaxWithKillersAndHash(&new_pos, depth - 1, -beta, -alpha,
                                         hash_table, new_zobrist, killers, 1, 1);


    if (eval > best_eval) {
      best_eval = eval;
      best_move = moves[i];
    }

    if (eval > alpha) {
      alpha = eval;
    }
  }

  //free killer move table
  //free(killers);

  return best_move;
}
