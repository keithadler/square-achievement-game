/* Square achievement game solver, multi-word bitboards (n up to 16). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint64_t u64;
#define W 4
typedef struct { u64 w[W]; } BB;

static int N, NC, NSQ;
static BB sqmask[1024], FULL;
static int cellsq[256][256], ncellsq[256];
static int perm[8][256], nperm;
static int SYMPLY = 12;
static long long nodes = 0;

static inline void bclr(BB*a){ for(int i=0;i<W;i++) a->w[i]=0; }
static inline void bset(BB*a,int c){ a->w[c>>6] |= 1ULL<<(c&63); }
static inline int  bget(const BB*a,int c){ return (a->w[c>>6]>>(c&63))&1; }
static inline int  bpop(const BB*a){ int s=0; for(int i=0;i<W;i++) s+=__builtin_popcountll(a->w[i]); return s; }
static inline int  bbzero(const BB*a){ for(int i=0;i<W;i++) if(a->w[i]) return 0; return 1; }
static inline BB   bor(BB a,BB b){ for(int i=0;i<W;i++) a.w[i]|=b.w[i]; return a; }
static inline BB   band(BB a,BB b){ for(int i=0;i<W;i++) a.w[i]&=b.w[i]; return a; }
static inline BB   bandn(BB a,BB b){ for(int i=0;i<W;i++) a.w[i]&=~b.w[i]; return a; } /* a & ~b */
static inline int  beq(const BB*a,const BB*b){ for(int i=0;i<W;i++) if(a->w[i]!=b->w[i]) return 0; return 1; }
static inline int  bbcmp(const BB*a,const BB*b){ for(int i=W-1;i>=0;i--){ if(a->w[i]<b->w[i]) return -1; if(a->w[i]>b->w[i]) return 1; } return 0; }
static inline int  bsub(const BB*a,const BB*b){ /* a subset-of b ? i.e. (b&a)==a */
    for(int i=0;i<W;i++) if((b->w[i]&a->w[i])!=a->w[i]) return 0; return 1; }
static inline int  bdis(const BB*a,const BB*b){ for(int i=0;i<W;i++) if(a->w[i]&b->w[i]) return 0; return 1; }

static void build(void){
    NC=N*N; NSQ=0; bclr(&FULL);
    for(int c=0;c<NC;c++) bset(&FULL,c);
    for(int s=1;s<N;s++) for(int r=0;r+s<N;r++) for(int c=0;c+s<N;c++){
        BB m; bclr(&m);
        bset(&m,r*N+c); bset(&m,r*N+c+s); bset(&m,(r+s)*N+c); bset(&m,(r+s)*N+c+s);
        sqmask[NSQ++]=m;
    }
    for(int c=0;c<NC;c++){ ncellsq[c]=0;
        for(int i=0;i<NSQ;i++) if(bget(&sqmask[i],c)) cellsq[c][ncellsq[c]++]=i; }
    nperm=0;
    for(int t=0;t<2;t++) for(int rot=0;rot<4;rot++){
        int *p=perm[nperm];
        for(int r=0;r<N;r++) for(int c=0;c<N;c++){
            int rr=r,cc=c,tmp;
            if(t){tmp=rr;rr=cc;cc=tmp;}
            for(int k=0;k<rot;k++){tmp=rr;rr=cc;cc=N-1-tmp;}
            p[r*N+c]=rr*N+cc;
        }
        nperm++;
    }
}
static BB xform(BB b,const int*p){
    BB o; bclr(&o);
    for(int i=0;i<W;i++){ u64 x=b.w[i];
        while(x){ int k=__builtin_ctzll(x); x&=x-1; bset(&o,p[(i<<6)+k]); } }
    return o;
}
static void canon(BB me,BB opp,BB*a,BB*b){
    BB ba=me,bb=opp;
    for(int g=1;g<nperm;g++){
        BB x=xform(me,perm[g]);
        int c=bbcmp(&x,&ba);
        if(c>0) continue;
        BB y=xform(opp,perm[g]);
        if(c<0 || bbcmp(&y,&bb)<0){ ba=x; bb=y; }
    }
    *a=ba; *b=bb;
}
static inline u64 hk(const BB*a,const BB*b){
    u64 h=0x9E3779B97F4A7C15ULL;
    for(int i=0;i<W;i++){ h^=a->w[i]+0x9E3779B97F4A7C15ULL+(h<<6)+(h>>2); }
    for(int i=0;i<W;i++){ h^=b->w[i]+0xC2B2AE3D27D4EB4FULL+(h<<6)+(h>>2); }
    h^=h>>29; h*=0xBF58476D1CE4E5B9ULL; h^=h>>32; return h;
}
#define TTBITS 25
#define TTSIZE (1u<<TTBITS)
typedef struct { BB k1,k2; int8_t val,flag,rem,used; } TT;
static TT *tt;
#define F_EXACT 0
#define F_LOWER 1
#define F_UPPER 2

static int solve(BB me,BB opp,int alpha,int beta,int rem){
    nodes++;
    BB occ=bor(me,opp), empty=bandn(FULL,occ);
    if(bbzero(&empty)) return 0;
    /* immediate win for side to move */
    for(int i=0;i<W;i++){ u64 x=empty.w[i];
        while(x){ int k=__builtin_ctzll(x); x&=x-1; int c=(i<<6)+k;
            for(int j=0;j<ncellsq[c];j++){ BB m=sqmask[cellsq[c][j]];
                BB t=bandn(m,me); if(bpop(&t)==1) return 1; } } }
    /* opponent immediate threats */
    BB threats; bclr(&threats); int nth=0;
    for(int i=0;i<NSQ;i++){ BB m=sqmask[i];
        if(!bdis(&m,&me)) continue;
        BB o=band(opp,m);
        if(bpop(&o)==3){ BB t=bandn(m,o);
            if(bdis(&t,&threats)){ threats=bor(threats,t); if(++nth>=2) return -1; } } }
    if(rem<=0) return 0;

    int nst=bpop(&occ);
    BB k1,k2;
    if(nst<=SYMPLY) canon(me,opp,&k1,&k2); else { k1=me; k2=opp; }
    u64 h=hk(&k1,&k2);
    TT*sl=&tt[h&(TTSIZE-1)];
    int a0=alpha;
    if(sl->used && beq(&sl->k1,&k1) && beq(&sl->k2,&k2) && sl->rem>=rem){
        if(sl->flag==F_EXACT) return sl->val;
        if(sl->flag==F_LOWER && sl->val>alpha) alpha=sl->val;
        else if(sl->flag==F_UPPER && sl->val<beta) beta=sl->val;
        if(alpha>=beta) return sl->val;
    }
    int cand[256],sc[256],nc=0;
    if(nth==1){ for(int i=0;i<W;i++) if(threats.w[i]){ cand[nc++]=(i<<6)+__builtin_ctzll(threats.w[i]); break; } }
    else {
        static const int wgt[4]={1,4,20,0};
        for(int i=0;i<W;i++){ u64 x=empty.w[i];
            while(x){ int k=__builtin_ctzll(x); x&=x-1; int c=(i<<6)+k; int s=0;
                for(int j=0;j<ncellsq[c];j++){ BB m=sqmask[cellsq[c][j]];
                    BB a=band(me,m), b=band(opp,m); int pm=bpop(&a),po=bpop(&b);
                    if(!po) s+=wgt[pm]; if(!pm) s+=wgt[po]; }
                sc[nc]=s; cand[nc]=c; nc++; } }
        for(int i=1;i<nc;i++){ int cv=cand[i],sv=sc[i],j=i-1;
            while(j>=0 && sc[j]<sv){ sc[j+1]=sc[j]; cand[j+1]=cand[j]; j--; }
            sc[j+1]=sv; cand[j+1]=cv; }
    }
    int best=-2;
    for(int i=0;i<nc;i++){
        BB nm=me; bset(&nm,cand[i]);
        int v=-solve(opp,nm,-beta,-alpha,rem-1);
        if(v>best) best=v;
        if(best>alpha) alpha=best;
        if(alpha>=beta) break;
    }
    int flag=(best<=a0)?F_UPPER:(best>=beta?F_LOWER:F_EXACT);
    int strem=rem;
    if((best==1&&flag!=F_UPPER)||(best==-1&&flag!=F_LOWER)) strem=120;
    if(!sl->used || !beq(&sl->k1,&k1) || !beq(&sl->k2,&k2) || strem>=sl->rem){
        sl->k1=k1; sl->k2=k2; sl->val=best; sl->flag=flag; sl->rem=strem; sl->used=1; }
    return best;
}
int main(int argc,char**argv){
    N=argc>1?atoi(argv[1]):6;
    int d0=argc>2?atoi(argv[2]):13;
    int d1=argc>3?atoi(argv[3]):d0;
    if(argc>4) SYMPLY=atoi(argv[4]);
    build();
    tt=calloc(TTSIZE,sizeof(TT));
    fprintf(stderr,"n=%d cells=%d squares=%d\n",N,NC,NSQ);
    for(int d=d0; d<=d1; d+=2){
        nodes=0; BB z; bclr(&z);
        int v=solve(z,z,-1,1,d);
        fprintf(stderr,"depth %2d: value %+d  nodes %lld\n",d,v,nodes); fflush(stderr);
        if(v!=0){ printf("n=%d RESULT: %s within %d plies\n",N,v>0?"FIRST PLAYER WINS":"SECOND PLAYER WINS",d); return 0; }
    }
    printf("n=%d: no decision in range\n",N); return 0;
}
