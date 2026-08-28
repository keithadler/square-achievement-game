/* Independent re-implementation: array board, no transposition table,
   no symmetry reduction, no move ordering, threat logic re-derived. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int N, NC, NSQ;
static int sq[1024][4];
static int board[256];          /* 0 empty, 1 = P1, 2 = P2 */
static long long nodes;

static void build(void){
    NC = N*N; NSQ = 0;
    for (int s = 1; s < N; s++)
      for (int r = 0; r+s < N; r++)
        for (int c = 0; c+s < N; c++){
            sq[NSQ][0]=r*N+c; sq[NSQ][1]=r*N+c+s;
            sq[NSQ][2]=(r+s)*N+c; sq[NSQ][3]=(r+s)*N+c+s; NSQ++;
        }
}

/* does colour p own all four cells of some square? */
static int owns(int p){
    for (int i = 0; i < NSQ; i++)
        if (board[sq[i][0]]==p && board[sq[i][1]]==p && board[sq[i][2]]==p && board[sq[i][3]]==p)
            return 1;
    return 0;
}

/* can colour p complete a square by playing empty cell c? */
static int completes(int p, int c){
    board[c] = p;
    int w = owns(p);
    board[c] = 0;
    return w;
}

static int val(int me, int rem, int alpha, int beta){
    nodes++;
    int opp = 3 - me;
    int nempty = 0;
    for (int c = 0; c < NC; c++) if (!board[c]) nempty++;
    if (!nempty) return 0;

    for (int c = 0; c < NC; c++)
        if (!board[c] && completes(me, c)) return 1;

    int th[256], nth = 0;
    for (int c = 0; c < NC; c++)
        if (!board[c] && completes(opp, c)) th[nth++] = c;
    if (nth >= 2) return -1;
    if (rem <= 0) return 0;

    int best = -2;
    if (nth == 1){
        board[th[0]] = me;
        best = -val(opp, rem-1, -beta, -alpha);
        board[th[0]] = 0;
        return best;
    }
    for (int c = 0; c < NC; c++){
        if (board[c]) continue;
        board[c] = me;
        int v = -val(opp, rem-1, -beta, -alpha);
        board[c] = 0;
        if (v > best) best = v;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

int main(int argc, char **argv){
    N = argc > 1 ? atoi(argv[1]) : 6;
    int d = argc > 2 ? atoi(argv[2]) : 13;
    build();
    memset(board, 0, sizeof board);
    fprintf(stderr, "sq2 n=%d cells=%d squares=%d depth=%d\n", N, NC, NSQ, d);
    nodes = 0;
    int v = val(1, d, -1, 1);
    printf("sq2 n=%d depth=%d value=%+d nodes=%lld\n", N, d, v, nodes);
    return 0;
}
