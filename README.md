# Checkmate

Checkmate is a minimalist chess engine written in C.\
It speaks the **UCI protocol** and focuses on tactical sharpness rather
than deep positional understanding. The codebase is intentionally
compact, making it suitable for learning, research, and lightweight
play.

## Features

-   **UCI support** -- compatible with GUIs such as Arena, CuteChess, or
    ChessGUI\
-   **Move generation** -- legal moves including castling, en passant,
    and promotions\
-   **Search** -- alpha--beta pruning with quiescence extensions\
-   **Transposition tables** -- Zobrist hashing with exact, lower, and
    upper bounds\
-   **Evaluation** -- material balance with basic heuristics and
    piece-square tables\
-   **FEN parser** -- load and analyze arbitrary positions

## Playing Style

-   Strong tactically: sharp in open positions, punishes blunders
    quickly\
-   Limited positional depth: struggles with long-term pawn structures
    and strategic maneuvering\
-   Practical strength: effective in fast time controls and
    tactics-heavy games

## Build

``` bash
git clone https://github.com/yourusername/checkmate
cd checkmate
gcc engine.c -o checkmate
```

## Estimated Strength

-   \~1900--2100 Elo depending on hardware and time control\
-   Competitive with hobby engines; below modern top-tier engines like
    Stockfish

## Roadmap

-   Improved positional evaluation (king safety, pawn structure,
    mobility)\
-   Opening book integration\
-   Endgame refinements / tablebase support

## License

MIT License
