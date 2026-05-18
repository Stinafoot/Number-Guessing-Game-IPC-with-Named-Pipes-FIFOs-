# Number Guessing Game - IPC with Named Pipes (FIFOs)

A multi-process number guessing game written in C that uses **named pipes (FIFOs)** for inter-process communication (IPC). Three separate processes - a referee and two players - run concurrently and communicate entirely through the filesystem. The referee generates random target numbers, both players compete to guess each one, and scores are tracked across 10 rounds.

---

## How It Works

### Architecture

The system is made up of three independent processes that must all be running simultaneously:

```
          ┌─────────────────────────────────┐
          │            REFEREE              │
          │  generates target, tracks score │
          └────────┬──────────┬─────────────┘
                   │          │
         ┌─────────┤          ├─────────┐
         │  FIFOs  │          │  FIFOs  │
         │         │          │         │
  ┌──────▼──────┐  │          │  ┌──────▼──────┐
  │  PLAYER 1   │  │          │  │  PLAYER 2   │
  │  Binary     │  │          │  │  Random     │
  │  Search     │  │          │  │  Guess      │
  └─────────────┘  │          │  └─────────────┘
                   │          │
              fifo_r_p1   fifo_r_p2   (referee → player)
              fifo_p1_r   fifo_p2_r   (player → referee)
```

### Communication Protocol

Each player communicates with the referee over a dedicated pair of FIFOs:

| FIFO | Direction | Contents |
|------|-----------|----------|
| `fifo_r_p1` | Referee → Player 1 | Start signal (`'S'`), then feedback (`int`) |
| `fifo_p1_r` | Player 1 → Referee | Guesses (`int`) |
| `fifo_r_p2` | Referee → Player 2 | Start signal (`'S'`), then feedback (`int`) |
| `fifo_p2_r` | Player 2 → Referee | Guesses (`int`) |

**Per-round message sequence:**
```
Referee  ──"S"──►  Player       (start signal, 1 byte)
Player   ──guess──► Referee      (integer guess)
Referee  ──cmp──►  Player        (comparison result: <0 high, 0 correct, >0 low)
          ... repeat until correct ...
```

The comparison value sent back is `guess - target`:
- `cmp < 0` → guess is **too low**, raise the minimum bound
- `cmp == 0` → **correct**, round ends
- `cmp > 0` → guess is **too high**, lower the maximum bound

---

## The Processes

### `referee.c` - Game Master
- Seeds a random number generator and runs **10 rounds**
- Picks a random target between 1 and 100 each round
- Opens all four FIFOs and manages both players simultaneously
- Each tick: reads one guess from each player, sends back comparison feedback
- If either player guesses correctly, awards them a point and ends the round
- Both players can score in the same round if they both guess correctly on the same tick
- Prints per-round scores and a final leaderboard to stdout using raw `write()` calls (no `printf`)

### `player1.c` - Binary Search Player
Uses a **binary search** strategy: always guesses the midpoint of the current valid range.

```
Initial range: [0, 100]
Guess = (min + max) / 2

If too low:  min = guess + 1
If too high: max = guess - 1
```

This guarantees finding any number in [1, 100] in at most **7 guesses** (⌈log₂(101)⌉).

### `player2.c` - Random Guess Player
Uses a **random guessing** strategy within the known valid range:

```
Guess = min + rand() % (max - min + 1)
```

Narrows bounds the same way as Player 1, but picks randomly within the remaining range rather than optimally. Seeds with `time(NULL)` for variation between runs.

---

## Performance Comparison

| Strategy | Worst case (range 1–100) | Average case |
|----------|--------------------------|--------------|
| Player 1 (Binary Search) | 7 guesses | ~6–7 guesses |
| Player 2 (Random) | Potentially many | ~50 guesses naively, fewer with bound-narrowing |

Player 1 will win most rounds due to the deterministic efficiency of binary search.

---

## Building

Compile each process separately:

```bash
gcc player1.c -o player1
gcc player2.c -o player2
gcc referee.c -o referee
```

Or all at once:

```bash
gcc player1.c -o player1 && gcc player2.c -o player2 && gcc referee.c -o referee
```

---

## Running

Each process must be launched in a **separate terminal** (or backgrounded). The players must be started before the referee, since opening a FIFO blocks until both ends are connected.

**Terminal 1:**
```bash
./player1
```

**Terminal 2:**
```bash
./player2
```

**Terminal 3:**
```bash
./referee
```

The referee will run 10 rounds automatically and print results as it goes.

### Example Output
```
Game 1
Score
P1: 1
P2: 0

Game 2
Score
P1: 1
P2: 1

...

Final Score
P1: 7
P2: 3
```

### Cleanup

The FIFOs are created as filesystem objects and persist after the processes exit. Remove them with:

```bash
rm -f fifo_r_p1 fifo_p1_r fifo_r_p2 fifo_p2_r
```

---

## File Structure

```
.
├── referee.c     # Game master: generates targets, scores, manages I/O
├── player1.c     # Binary search guesser
└── player2.c     # Random range guesser
```

---

## Key Concepts Demonstrated

- **Named pipes (FIFOs):** `mkfifo()`, `open()`, `read()`, `write()` for IPC across independent processes
- **Concurrent processes:** three processes running simultaneously, synchronized purely through blocking pipe I/O
- **Binary search algorithm:** deterministic O(log n) strategy applied to interactive guessing
- **Randomized algorithms:** bounded random guessing with progressive range narrowing
- **POSIX system calls:** low-level I/O without the C standard library (`write()` instead of `printf`)
- **Error handling:** `errno` checking, `EEXIST` tolerance for pre-existing FIFOs

---

## Requirements

- Linux / Unix environment (POSIX FIFOs required)
- GCC or any C compiler supporting C99 or later
