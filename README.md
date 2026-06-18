# BMDImpt: Implementing *BMD Based on RicBDD

This repository contains **BMDImpt**, a final project implementation of a
functional Multiplicative Binary Moment Diagram (`*BMD`) engine inside the
RicBDD infrastructure.

`*BMD` is a word-level decision diagram for functions from Boolean inputs to
integer values.  Unlike ordinary BDDs, which are mainly suitable for bit-level
Boolean functions, `*BMD` uses the arithmetic positive Davio / moment
decomposition:

```text
f = f0 + x * (f1 - f0)
```

This makes it particularly suitable for arithmetic circuits and RTL-style
word-level designs, such as adders and multipliers.

## Group Members

Please fill in before submission:

- TODO: Name, e-mail, and backup contact information
- TODO: Name, e-mail, and backup contact information, if this is a two-person team

## Build, Run, and Test

### Build

```sh
make clean
make depend
make
```

The generated executable is:

```sh
./testBdd
```

### Run Benchmarks

```sh
./testBdd
```

The benchmark prints:

- engine name: `BDD` or `*BMD`
- benchmark circuit
- bit width
- node count
- estimated memory usage
- runtime
- sample correctness check

### Clean Generated Files

```sh
make clean
rm -f .depend.mak
```

Generated object files, executable binaries, dependency files, crash dumps, and
local diagram files are ignored by `.gitignore` and should not be pushed.

## Key Research Contributions

This project contributes the following items on top of the original RicBDD
package:

- A new `*BMD` engine implemented in the same lightweight C++ style as RicBDD.
- A `BmdNode` weighted-pair representation for integer-valued functions.
- A `BmdMgr` manager with unique table, computed cache, and arithmetic apply
  operations.
- Positive Davio / moment decomposition for arithmetic functions.
- GCD-based edge-weight normalization for canonical `*BMD` construction.
- Arithmetic operations: addition, subtraction, and multiplication.
- Boolean operations represented arithmetically: NOT, AND, OR, and XOR.
- A benchmark driver comparing RicBDD's original BDD engine with the new
  `*BMD` engine using node count, estimated memory, and runtime.
- A written analysis of why `*BMD` is more suitable for word-level arithmetic
  circuits.

## Prior Work and Non-Contributions

The following work should **not** be counted as this project's original
contribution:

- The original RicBDD package and its BDD implementation:
  - `bddNode.h`, `bddNode.cpp`
  - `bddMgr.h`, `bddMgr.cpp`
  - `myHash.h`
  - `myString.cpp`
  - `Makefile`
- The theoretical `*BMD` data structure and algorithms introduced by Bryant and
  Chen.
- The general idea of using BDDs and `*BMDs` for arithmetic circuit
  verification.

This project does not use any publicly accessible third-party framework beyond
the original RicBDD source code and the C++ standard library.

Small changes were made to RicBDD's BDD manager only to expose benchmark
statistics and to make repeated benchmark construction stable.

## Implemented Files

- `bmdNode.h`, `bmdNode.cpp`  
  External `*BMD` handle and internal vertex data structure.

- `bmdMgr.h`, `bmdMgr.cpp`  
  `*BMD` manager, unique table, computed cache, construction routines, and
  apply algorithms.

- `testBdd.cpp`  
  Benchmark driver for comparing `BDD` and `*BMD`.

- `BMD_REPORT.md`  
  Supplementary implementation notes and experiment summary.

- `.gitignore`  
  Prevents generated object files, binaries, dependency files, crash dumps, and
  local diagrams from being pushed.

## Algorithm Pseudo Code

### Weighted Pair

Each `*BMD` function is represented as:

```text
pair = <weight, vertex>
```

If `vertex` is the terminal vertex, `weight` is the constant value.
Otherwise, the pair represents:

```text
weight * value(vertex)
```

Each non-terminal vertex stores:

```text
level
lo = constant moment
hi = linear moment
```

The represented function is:

```text
lo + x_level * hi
```

### ApplyWeight

```text
ApplyWeight(w, <a, v>):
    if w == 0 or a == 0:
        return <0, terminal>
    return <w * a, v>
```

### MakeBranch

```text
MakeBranch(level, lo, hi):
    if hi.weight == 0:
        return lo

    g = gcd(lo.weight, hi.weight)

    if lo.weight < 0 or (lo.weight == 0 and hi.weight < 0):
        g = -g

    lo.weight = lo.weight / g
    hi.weight = hi.weight / g

    vertex = UniqueVertex(level, lo, hi)
    return <g, vertex>
```

This follows the canonicalization idea in Bryant and Chen's `*BMD`
construction.  The zero-linear-moment rule removes variables that do not affect
the function.

### PlusApply

```text
PlusApply(f, g):
    if f == 0:
        return g
    if g == 0:
        return f
    if f.vertex == g.vertex:
        return ApplyWeight(f.weight + g.weight, <1, f.vertex>)

    normalize argument order
    if result exists in computed cache:
        return cached result

    x = top variable among f and g

    f_lo = SimpleMoment(f, x, constant)
    f_hi = SimpleMoment(f, x, linear)
    g_lo = SimpleMoment(g, x, constant)
    g_hi = SimpleMoment(g, x, linear)

    lo = PlusApply(f_lo, g_lo)
    hi = PlusApply(f_hi, g_hi)

    result = MakeBranch(x, lo, hi)
    cache and return result
```

### MultApply

For positive Davio form:

```text
f = f0 + x * f'
g = g0 + x * g'
```

Since `x` is Boolean, `x^2 = x`:

```text
f * g = f0*g0 + x * (f'*g0 + f0*g' + f'*g')
```

Pseudo code:

```text
MultApply(f, g):
    if f == 0 or g == 0:
        return 0
    if f is terminal:
        return ApplyWeight(f.weight, g)
    if g is terminal:
        return ApplyWeight(g.weight, f)

    normalize argument order
    if result exists in computed cache:
        return cached result

    x = top variable among f and g

    f_lo = SimpleMoment(f, x, constant)
    f_hi = SimpleMoment(f, x, linear)
    g_lo = SimpleMoment(g, x, constant)
    g_hi = SimpleMoment(g, x, linear)

    lo = MultApply(f_lo, g_lo)
    hi = PlusApply(
             MultApply(f_hi, g_lo),
             PlusApply(MultApply(f_lo, g_hi),
                       MultApply(f_hi, g_hi)))

    result = MakeBranch(x, lo, hi)
    cache and return result
```

### Boolean Operations

Boolean functions are treated as arithmetic 0/1 functions:

```text
NOT(f)  = 1 - f
AND(f,g)= f * g
OR(f,g) = f + g - f*g
XOR(f,g)= f + g - 2*f*g
```

## Noticeable Implementation Details

- The implementation uses the original RicBDD `Hash` and `Cache` templates.
- `BmdNode` stores the root weight and a pointer to `BmdNodeInt`, matching the
  weighted-pair abstraction from the papers.
- `BmdNodeInt` stores `lo` and `hi` as weighted `BmdNode` objects.
- All support variables are built as:

```text
x_i = MakeBranch(i, 0, 1)
```

- The benchmark uses interleaved variable ordering:

```text
x0, y0, x1, y1, ...
```

This avoids unfairly penalizing BDDs for equality and addition.

- Estimated memory is computed as:

```text
node_count * sizeof(NodeType)
```

This is not an exact heap profiler measurement, but it gives a consistent
comparison between engines.

- Current numeric weights use `long long`.  This is sufficient for the included
  benchmarks but should be replaced by arbitrary precision integers for a more
  robust PR-quality implementation.

## Experimental Results

The latest benchmark run passed all sample correctness checks.

Representative results:

```text
Engine  Circuit       Bits       Nodes   Memory(est)     Seconds  Check
------------------------------------------------------------------------
*BMD    encode          32          33          1584    0.000113  pass
*BMD    add             32          65          3120    0.000189  pass
*BMD    multiply        16          63          3024    0.000114  pass
BDD     equality        16          95          2280    0.000050  pass
BDD     add             16         171          4104    0.000120  pass
BDD     multiply         8       55996       1343904    0.102797  pass
```

Observations:

- `*BMD` represents unsigned word encoding linearly.
- `*BMD` addition also grows linearly in this benchmark.
- `*BMD` multiplication remains compact when constructed at the word level.
- RicBDD's BDD engine handles equality and addition well with interleaved
  variable ordering.
- Bit-level BDD multiplication grows much faster, reaching 55,996 nodes for
  only 8-bit multiplication.

These results are consistent with the motivation of `*BMD`: BDDs are useful for
Boolean control logic, while `*BMDs` are more suitable for word-level arithmetic
circuits such as RTL adders and multipliers.

## Limitations and Future Work

- Add arbitrary precision integer support for edge weights.
- Add DOT dumping for `*BMD` graphs.
- Add unit tests instead of relying only on the benchmark driver.
- Add BLIF/AIG/ISCAS benchmark parsing.
- Add hierarchical arithmetic circuit verification, following the ACV-style
  methodology described in prior work.
- Package the implementation as a cleaner pull request to the upstream RicBDD
  repository.

## Demo Video

TODO: Add demo video link before final submission.

Recommended demo content:

1. Show repository structure.
2. Run `make clean && make depend && make`.
3. Run `./testBdd`.
4. Explain the benchmark output.
5. Briefly explain why `*BMD` multiplication has far fewer nodes than BDD
   bit-level multiplication.

## References

1. R. E. Bryant and Y.-A. Chen, "Verification of Arithmetic Circuits with
   Binary Moment Diagrams," DAC 1995.
2. R. E. Bryant and Y.-A. Chen, "Verification of Arithmetic Functions with
   Binary Moment Diagrams," CMU Technical Report, 1994.
3. K. Hamaguchi, A. Morita, and S. Yajima, "Efficient Construction of Binary
   Moment Diagrams for Verifying Arithmetic Circuits," ICCAD 1995.
4. R. E. Bryant, "Graph-Based Algorithms for Boolean Function Manipulation,"
   IEEE Transactions on Computers, 1986.

## Optional Course Comments

TODO: Add optional comments about this course, if desired.
