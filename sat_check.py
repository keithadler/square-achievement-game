#!/usr/bin/env python3
"""Validate the square enumeration against Bacher-Eliahou.

Encodes "does a square-free 2-colouring of the n x n grid exist?" as SAT, using
the same enumeration of axis-aligned squares that the C solvers use. Bacher and
Eliahou (2010) proved the answer is yes for n <= 14 and no for n >= 15.

    pip install python-sat
    python3 sat_check.py 12 13 14
"""
import sys, time
from pysat.solvers import Minisat22

def squares(n):
    return [[r*n+c, r*n+c+s, (r+s)*n+c, (r+s)*n+c+s]
            for s in range(1, n) for r in range(n-s) for c in range(n-s)]

for n in (int(a) for a in (sys.argv[1:] or ["12", "13", "14"])):
    S = squares(n); t = time.time()
    with Minisat22() as m:
        for q in S:
            m.add_clause([-(v+1) for v in q])   # not all one colour
            m.add_clause([ (v+1) for v in q])   # not all the other
        sat = m.solve()
    print(f"n={n:2d}  squares={len(S):5d}  square-free 2-colouring exists: {sat}"
          f"  ({time.time()-t:.1f}s)")
