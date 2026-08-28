# Square achievement game on an n × n grid

**Result: the first player wins for every n ≥ 5, and from n = 6 onward the win is forced within 13 plies.**

Two players alternately mark cells of an n × n grid. The first to own four cells
at the corners of a square with horizontal and vertical sides wins. If the board
fills with neither player doing so, the game is a draw.

The problem was posed by Martin Erickson in 2010 and catalogued on
[Open Problem Garden](http://www.openproblemgarden.org/op/a_game_on_an_n_x_n_grid).
Its outcome was known only at the two ends of the range. This repository contains
solvers that settle the nine sizes in between, together with the logs they produced
and the checks run against them.

## State of the problem

| n | outcome | established by |
|---|---|---|
| ≤ 2 | draw | trivial |
| 3, 4 | draw | Jenrich 2012 |
| 5 | first player wins | Jenrich 2012 |
| **6 – 14** | **first player wins** | **this work** |
| ≥ 15 | first player wins | Bacher–Eliahou 2010, plus strategy stealing |

Bacher and Eliahou showed that no square-free 2-colouring of a 15 × 15 grid
exists, so from n = 15 the game cannot end drawn; strategy stealing then rules
out a second-player win. Below that, square-free end configurations do exist —
Bacher and Eliahou exhibit 232,228 of them on the 14 × 14 board — so for
n ≤ 14 the outcome genuinely had to be searched rather than argued.

## Results

Every one of the nine open sizes is a first-player win, forced within 13 plies.

| n | cells | squares | plies | search nodes |
|---:|---:|---:|---:|---:|
| 6 | 36 | 55 | 13 | 2,016,166 |
| 7 | 49 | 91 | 13 | 2,644,635 |
| 8 | 64 | 140 | 13 | 5,682,421 |
| 9 | 81 | 204 | 13 | 21,674,452 |
| 10 | 100 | 285 | 13 | 56,785,588 |
| 11 | 121 | 385 | 13 | 116,660,038 |
| 12 | 144 | 506 | 13 | 231,698,908 |
| 13 | 169 | 650 | 13 | 735,265,715 |
| 14 | 196 | 819 | 13 | 746,507,072 |

The depth of the win stops moving at n = 6. What grows is the cost of verifying
the second player's replies, not the length of the forced sequence.

## Why a shallow search settles it

Solving even the 36-cell game outright is out of reach, and the 196-cell game
hopelessly so. It is also unnecessary.

Run negamax with the search horizon scored as a **draw**. The value `+1` can then
only be returned when the side to move genuinely completes a square inside the
horizon — nothing else in the evaluation can produce it. So a depth-limited
search that reports a win **has proved a win**. Only a reported draw would be
inconclusive, and no case here needed one. Searching directly at depth 13 skips
the expensive depth-11 refutations entirely.

Three pruning rules do most of the work. Each is sound, not heuristic:

1. **Immediate win** — if some empty cell completes a square for the side to
   move, return a win without recursing.
2. **Two threats lose** — otherwise, if the opponent has two distinct cells that
   each complete a square for them, the position is lost: one can be blocked,
   not both.
3. **One threat forces** — if exactly one such cell exists, it is the only move
   that does not lose immediately, so branching collapses to one.

On top of that: bitboards (64-bit up to n = 8, 256-bit up to n = 14), a
transposition table, folding by the eight symmetries of the square, and move
ordering that favours cells lying in already half-owned squares.

## Reproducing

```sh
make            # builds all five binaries
make result     # re-runs n = 6..14, the headline result
make check      # re-runs the published cases n = 3, 4, 5
```

Individual runs take arguments `n`, start depth, end depth, and the ply count
below which positions are folded by symmetry:

```sh
./sq3 6 13 13 12      # n=6, search depth 13 only, symmetry folding under 12 plies
./sq3 6 13 13 0       # same, with symmetry folding disabled entirely
./sq4 6 13            # extract and certify the full winning strategy for n=6
```

`make result` takes roughly three hours on a 2023 laptop; n = 13 and n = 14
dominate that. The smaller cases finish in seconds.

## What was checked

**The square enumeration is validated against an external theorem.** Every
result here rests on the list of axis-aligned squares the solvers enumerate. That
same list, encoded as SAT ("does a square-free 2-colouring of the n × n grid
exist?"), reproduces Bacher and Eliahou's threshold: satisfiable for
n = 12, 13, 14, which is their published finding that the 14 × 14 board still
admits square-free colourings. This checks the geometric data against outside
work rather than against more of my own code. See `sat_check.py`.

**Reproduces the literature.** All three published values come back correct:
n = 3 and 4 draw, n = 5 a first-player win — and the n = 5 win first appears at
depth 17, not earlier, which is a sharper test than the value alone.

**Two engines agree.** `sq.c` (64-bit bitboards) and `sq3.c` (256-bit) were
written separately and return identical values for n = 6, 7, 8.

**A deliberately naive engine agrees.** `sq2.c` uses an array board with no
transposition table, no symmetry folding and no move ordering. It confirms the
n = 4 draw over 63,492,032 nodes.

**Optimisations removed.** `sq3strict.c` disables both symmetry folding and the
depth-independent transposition-table storage — the two places where a bug could
plausibly manufacture a win. Re-run in that configuration, n = 6..11 return the
same answers at substantially higher node counts:

| n | normal | strict |
|---:|---:|---:|
| 6 | 2,016,166 | 5,152,122 |
| 7 | 2,644,635 | 22,782,372 |
| 8 | 5,682,421 | 26,380,743 |
| 9 | 21,674,452 | 119,962,990 |
| 10 | 56,785,588 | 130,171,084 |
| 11 | 116,660,038 | 658,263,739 |

**The strategy is certified, not just scored.** `sq4.c` walks an entire winning
strategy tree — every first-player move and *every* legal reply at every node —
and verifies that each leaf is a genuinely completed square owned by the first
player. It aborts if the second player could ever complete a square first.
Neither run aborted.

| n | decision nodes | winning leaves |
|---:|---:|---:|
| 6 | 56,857,431 | 54,480,689 |
| 7 | 412,121,227 | 400,880,087 |

This is the strongest check here, because it does not rely on the pruning rules
at all: it terminates only on squares it can see on the board. Two certificates
at different board sizes are much harder to explain away as a solver artefact
than one.

## Files

| file | what it is |
|---|---|
| `sq.c` | 64-bit bitboard solver, n ≤ 8 |
| `sq3.c` | 256-bit bitboard solver, n ≤ 14 |
| `sq2.c` | deliberately naive cross-check, no TT / symmetry / ordering |
| `sq3strict.c` | `sq3.c` with depth-independent TT storage removed |
| `sq4.c` | strategy extractor and certifier |
| `RESULTS.txt` | collected results and per-run node counts |
| `*.log` | raw solver output from the runs quoted above |

## Caveats

These results are machine-checked but have **not** been refereed, and the
checks above are all my own code checking my own code. The certificate for
n = 6 and n = 7 are the strongest evidence, because they verify completed
squares directly rather than trusting the search. Independent reproduction is
genuinely wanted — the whole point of the MIT licence here is that anyone can
take these four files and confirm or demolish the claim.

## References

- Martin Erickson, *Pearls of Discrete Mathematics*, CRC Press, 2010. Problem posed at [Open Problem Garden](http://www.openproblemgarden.org/op/a_game_on_an_n_x_n_grid).
- Thomas Jenrich, *Guaranteed successful strategies for a square achievement game on an n by n grid*, [arXiv:1109.2341](https://arxiv.org/abs/1109.2341).
- Roland Bacher and Shalom Eliahou, *Extremal binary matrices without constant 2-squares*, Journal of Combinatorics **1** (2010), 77–100.

## Licence

MIT — see [LICENSE](LICENSE).
