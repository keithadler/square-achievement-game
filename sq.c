/* Square achievement game on an n x n grid.
   Two players alternately mark cells; first to own 4 cells forming an
   axis-aligned square wins. Otherwise draw.  Exact negamax solver. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint64_t u64;
static int N, NC, NSQ;
static u64 sqmask[512];
static int cellsq[64][64], ncellsq[64];
static int perm[8][64], nperm;
static int SYMPLY = 12;

static long long nodes = 0;

#define TTBITS 24
#define TTSIZE (1u<<TTBITS)
typedef struct { u64 k1, k2; int8_t val, flag, rem; } TT;
static TT *tt;
#define F_EXACT 0
#define F_LOWER 1
#define F_UPPER 2

static void build(void){
    NC = N*N; NSQ = 0;
    for (int s = 1; s < N; s++)
        for (int r = 0; r + s < N; r++)
            for (int c = 0; c + s < N; c++){
                u64 m = 0;
                m |= 1ULL << (r*N+c);
                m |= 1ULL << (r*N+c+s);
                m |= 1ULL << ((r+s)*N+c);
                m |= 1ULL << ((r+s)*N+c+s);
                sqmask[NSQ++] = m;
            }
    for (int c = 0; c < NC; c++){
        ncellsq[c] = 0;
        for (int i = 0; i < NSQ; i++)
            if (sqmask[i] >> c & 1) cellsq[c][ncellsq[c]++] = i;
    }
    nperm = 0;
    for (int t = 0; t < 2; t++) for (int rot = 0; rot < 4; rot++){
        int *p = perm[nperm];
        for (int r = 0; r < N; r++) for (int c = 0; c < N; c++){
            int rr = r, cc = c, tmp;
            if (t) { tmp = rr; rr = cc; cc = tmp; }
            for (int k = 0; k < rot; k++){ tmp = rr; rr = cc; cc = N-1-tmp; }
            p[r*N+c] = rr*N+cc;
        }
        nperm++;
    }
}

static inline u64 xform(u64 b, const int *p){
    u64 o = 0;
    while (b){ int c = __builtin_ctzll(b); b &= b-1; o |= 1ULL << p[c]; }
    return o;
}

static void canon(u64 me, u64 opp, u64 *a, u64 *b){
    u64 ba = me, bb = opp;
    for (int g = 1; g < nperm; g++){
        u64 x = xform(me, perm[g]), y = xform(opp, perm[g]);
        if (x < ba || (x == ba && y < bb)){ ba = x; bb = y; }
    }
    *a = ba; *b = bb;
}

static inline u64 hashkey(u64 a, u64 b){
    u64 h = a * 0x9E3779B97F4A7C15ULL;
    h ^= b + 0xC2B2AE3D27D4EB4FULL + (h<<6) + (h>>2);
    h ^= h >> 29; h *= 0xBF58476D1CE4E5B9ULL; h ^= h >> 32;
    return h;
}

/* value from perspective of side to move: 1 win, 0 draw, -1 loss */
static int solve(u64 me, u64 opp, int alpha, int beta, int rem){
    nodes++;
    u64 occ = me | opp;
    u64 empty = (NC==64 ? ~0ULL : ((1ULL<<NC)-1)) & ~occ;
    if (!empty) return 0;

    /* immediate win? */
    u64 e = empty;
    while (e){
        int c = __builtin_ctzll(e); e &= e-1;
        u64 nb = me | (1ULL<<c);
        for (int i = 0; i < ncellsq[c]; i++){
            u64 m = sqmask[cellsq[c][i]];
            if ((nb & m) == m) return 1;
        }
    }
    /* opponent immediate threats */
    u64 threats = 0; int nth = 0;
    for (int i = 0; i < NSQ; i++){
        u64 m = sqmask[i];
        if (me & m) continue;
        u64 o = opp & m;
        if (__builtin_popcountll(o) == 3){
            u64 t = m & ~o;
            if (!(threats & t)) { threats |= t; nth++; if (nth >= 2) return -1; }
        }
    }
    if (rem <= 0) return 0;

    int nstones = __builtin_popcountll(occ);
    u64 k1, k2;
    if (nstones <= SYMPLY) canon(me, opp, &k1, &k2); else { k1 = me; k2 = opp; }
    u64 h = hashkey(k1, k2);
    TT *slot = &tt[h & (TTSIZE-1)];
    int a0 = alpha;
    if (slot->k1 == k1 && slot->k2 == k2){
        if (slot->rem >= rem){
            if (slot->flag == F_EXACT) return slot->val;
            if (slot->flag == F_LOWER && slot->val > alpha) alpha = slot->val;
            else if (slot->flag == F_UPPER && slot->val < beta) beta = slot->val;
            if (alpha >= beta) return slot->val;
        }
    }

    int cand[64], nc = 0;
    if (nth == 1) { cand[nc++] = __builtin_ctzll(threats); }
    else {
        int sc[64];
        u64 t = empty;
        while (t){
            int c = __builtin_ctzll(t); t &= t-1;
            int s = 0;
            for (int i = 0; i < ncellsq[c]; i++){
                u64 m = sqmask[cellsq[c][i]];
                int pm = __builtin_popcountll(me & m), po = __builtin_popcountll(opp & m);
                static const int w[4] = {1,4,20,0};
                if (!po) s += w[pm];
                if (!pm) s += w[po];
            }
            sc[nc] = s; cand[nc] = c; nc++;
        }
        for (int i = 1; i < nc; i++){
            int cv = cand[i], sv = sc[i], j = i-1;
            while (j >= 0 && sc[j] < sv){ sc[j+1]=sc[j]; cand[j+1]=cand[j]; j--; }
            sc[j+1]=sv; cand[j+1]=cv;
        }
    }

    int best = -2;
    for (int i = 0; i < nc; i++){
        int c = cand[i];
        int v = -solve(opp, me | (1ULL<<c), -beta, -alpha, rem-1);
        if (v > best) best = v;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }

    int flag = (best <= a0) ? F_UPPER : (best >= beta ? F_LOWER : F_EXACT);
    int strem = rem;
    if ((best == 1 && flag != F_UPPER) || (best == -1 && flag != F_LOWER)) strem = 120;
    if (slot->k1 != k1 || slot->k2 != k2 || strem >= slot->rem){
        slot->k1 = k1; slot->k2 = k2; slot->val = best; slot->flag = flag; slot->rem = strem;
    }
    return best;
}

int main(int argc, char **argv){
    N = argc > 1 ? atoi(argv[1]) : 5;
    int maxrem = argc > 2 ? atoi(argv[2]) : 0;
    if (argc > 3) SYMPLY = atoi(argv[3]);
    build();
    tt = calloc(TTSIZE, sizeof(TT));
    fprintf(stderr, "n=%d cells=%d squares=%d\n", N, NC, NSQ);
    if (!maxrem) maxrem = NC;
    for (int d = 1; d <= maxrem; d += 2){
        nodes = 0;
        int v = solve(0, 0, -1, 1, d);
        fprintf(stderr, "depth %2d: value %+d   nodes %lld\n", d, v, nodes);
        fflush(stderr);
        if (v != 0){
            printf("n=%d RESULT: %s (within %d plies)\n", N, v>0?"FIRST PLAYER WINS":"SECOND PLAYER WINS", d);
            return 0;
        }
        if (d >= NC){ printf("n=%d RESULT: DRAW (full search)\n", N); return 0; }
    }
    printf("n=%d: no result up to depth %d\n", N, maxrem);
    return 0;
}
