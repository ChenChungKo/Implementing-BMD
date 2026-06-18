# BMDImpt: *BMD Engine in RicBDD

## Overview

This project adds a functional Multiplicative Binary Moment Diagram (*BMD)
engine to the RicBDD code base.  The implementation follows the same small
manager/node style as RicBDD while using the arithmetic positive Davio form:

```text
f = f0 + x * (f1 - f0)
```

In the *BMD implementation, each function is represented as a weighted pair
`<weight, vertex>`.  A terminal pair stores a numeric constant directly in the
weight.  A non-terminal vertex stores the variable level and two weighted
children:

- `lo`: the constant moment `f0`
- `hi`: the linear moment `f1 - f0`

The manager normalizes branch weights with `gcd(lo.weight, hi.weight)` before
inserting a vertex into the unique table.  This is the canonicalization rule
described by Bryant and Chen for integer-valued *BMDs.

## Implemented Features

- `BmdNode` and `BmdMgr` integrated alongside existing RicBDD classes.
- Integer constants and Boolean support variables.
- Positive Davio `makeBranch()` with zero-linear-moment reduction.
- Multiplicative edge/root weights through `applyWeight()`.
- Arithmetic operations: addition, subtraction, multiplication.
- Boolean operations represented arithmetically:
  - NOT: `1 - f`
  - AND: `f * g`
  - OR: `f + g - f * g`
  - XOR: `f + g - 2 * f * g`
- Computed cache for addition and multiplication.
- Evaluation on input bit patterns.
- Node count and estimated memory reporting.
- Benchmark driver comparing *BMD against RicBDD's BDD engine.

## Files

- `bmdNode.h`, `bmdNode.cpp`: external *BMD handle and internal vertex.
- `bmdMgr.h`, `bmdMgr.cpp`: unique table, computed cache, construction and apply algorithms.
- `testBdd.cpp`: benchmark driver for BMDImpt.
- `bddMgr.h`, `bddMgr.cpp`: small additions for benchmark statistics and safer repeated BDD benchmarking.

## How to Build and Run

```sh
make clean
make depend
make
./testBdd
```

The benchmark prints:

- engine name
- benchmark circuit
- bit width
- node count
- estimated memory
- runtime
- sample correctness check

## Benchmarks

The current driver covers:

- *BMD unsigned word encoding `X`
- *BMD word-level addition `X + Y`
- *BMD word-level multiplication `X * Y`
- BDD equality comparator
- BDD ripple-carry addition
- BDD bit-level array multiplication

Both BDD and *BMD benchmarks use interleaved variable ordering:

```text
x0, y0, x1, y1, ...
```

This avoids unfairly penalizing BDDs on equality and addition.

## Sample Results

On the current Raspberry Pi environment, the latest run passed all sample
correctness checks.  The most important observations were:

- *BMD word encoding grows linearly: 4-bit uses 5 nodes; 32-bit uses 33 nodes.
- *BMD addition grows linearly: 4-bit uses 9 nodes; 32-bit uses 65 nodes.
- *BMD multiplication also stays compact in this word-level construction:
  4-bit uses 15 nodes; 16-bit uses 63 nodes.
- BDD equality and addition are reasonable with interleaved ordering.
- BDD bit-level multiplication grows much faster:
  2-bit uses 17 nodes, 4-bit uses 405 nodes, 6-bit uses 5039 nodes,
  and 8-bit uses 55996 nodes.

These results match the paper's expectation: BDDs can work well for Boolean
control logic and some adders with a good variable order, while *BMDs are much
better aligned with word-level arithmetic functions such as multiplication.

## Limitations and Future Work

- Weights are currently `long long`, so very large arithmetic functions can overflow.
  A PR-quality version should use arbitrary precision integers or a configurable
  numeric type.
- The benchmark builds word-level *BMD specifications directly.  A full verifier
  should add circuit parsing and hierarchical module verification.
- The BDD benchmark keeps BDD managers alive until process exit because RicBDD's
  original static terminal handles assume singleton-like manager lifetime.
- More benchmark formats should be added, such as BLIF/AIG/ISCAS arithmetic
  circuits and synthesized RTL multiplier variants.
- A PR should add unit tests, DOT dumping for *BMDs, and cleaner Makefile targets.

## References

- R. E. Bryant and Y.-A. Chen, "Verification of Arithmetic Circuits with Binary
  Moment Diagrams," DAC 1995.
- R. E. Bryant and Y.-A. Chen, "Verification of Arithmetic Functions with Binary
  Moment Diagrams," CMU Technical Report, 1994.
- K. Hamaguchi, A. Morita, and S. Yajima, "Efficient Construction of Binary
  Moment Diagrams for Verifying Arithmetic Circuits," ICCAD 1995.
