# ♟️ Checkmate

Checkmate is a minimalist uci chess engine written in C.\
The codebase is compact, making it suitable for learning, research, and lightweight
play.

## Features

-   **UCI support** -- compatible with GUIs such as Arena, CuteChess or enCroissant\
-   **Move generation** -- legal moves including castling, en passant,
    and promotions\
-   **Search** -- Negamax with pruning and quiescence search\
-   **Transposition tables** -- Zobrist hashing with exact, lower, and
    upper bounds\
-   **Evaluation** -- material balance with basic heuristics and
    piece-square tables\
-   **FEN parser** -- load and analyze arbitrary positions

## Playing Style

-   Strong tactically: sharp in open positions
-   Limited positional depth: struggles with long-term pawn structures
    and strategic maneuvering
-   Practical strength: effective in fast time controls and
    tactics-heavy games

## Requirements
- Runs on most modern hardware (depth needs to be adjusted accordingly, iterative deepening ís not yet implemented)
- In theory, about 5MB of Ram is needed, i reccommend about 1-2GB if you plan to use zobrist transposition tables 
## Build

``` bash
git clone https://github.com/kuglatec/checkmate
cd checkmate
gcc engine.c -Ofast -o checkmate
```

## Estimated Strength
not measured yet D:
coming soon

## Roadmap

-   Improved positional evaluation (king safety, pawn structure,
    mobility, maybe NNUE)
-   Endgame refinements / tablebase support

## License
GPLv3
