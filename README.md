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

工海所 碩一 陳眾科 R14525042
Email: r14525042@ntu.edu.tw

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
- a small `*BMD` DOT graph example: `bmd_multiply_4.dot`
- a CSV result file: `benchmark_results.csv`

### Command-Line Options

By default, `./testBdd` runs the full benchmark suite.  You can also run a
single benchmark with CLI options:

```sh
./testBdd --engine bmd --circuit multiply --bits 16
./testBdd --engine bdd --circuit multiply --bits 8
./testBdd --engine bmd --circuit subtract --bits 32 --csv bmd_sub32.csv
```

Supported `*BMD` circuits:

```text
encode, add, subtract, multiply
```

Supported BDD circuits:

```text
equality, add, multiply
```

Other useful options:

```sh
./testBdd --help
./testBdd --all
./testBdd --csv benchmark_results.csv
./testBdd --dot bmd_multiply_4.dot
./testBdd --no-dot
```

### Restricted Verilog Arithmetic Input

The benchmark driver also supports a small restricted Verilog arithmetic input
mode:

```sh
./testBdd --verilog examples/add4.v
./testBdd --verilog examples/sub4.v
./testBdd --verilog examples/mul4.v
```

This is **not** a full Verilog frontend.  It is intended as a lightweight
word-level RTL arithmetic specification parser for simple modules of the form:

```verilog
module top(input [3:0] x, input [3:0] y, output [7:0] z);
assign z = x * y;
endmodule
```

Supported expressions are:

```text
assign z = x + y;
assign z = x - y;
assign z = x * y;
```

The parser extracts the input width and operation, then constructs the
corresponding `*BMD` word-level function.  This mirrors the common research
workflow where RTL arithmetic descriptions are interpreted or translated into a
word-level representation before comparison.  Full Verilog parsing, synthesis,
and AIG/BLIF conversion are left as future work.

The benchmark also runs an exhaustive `*BMD` correctness test for 4-bit
word-level encoding, addition, and multiplication.  This checks all input
combinations:

```text
X = 0..15
Y = 0..15
```

and verifies:

```text
encode(X)    = X
add(X, Y)    = X + Y
multiply(X,Y)= X * Y
```

The benchmark also exhaustively validates `*BMD` Boolean operations over all
2-input combinations:

```text
NOT, AND, OR, XOR
```

### Clean Generated Files

```sh
make clean
rm -f .depend.mak
```

Generated object files, executable binaries, dependency files, crash dumps, and
local diagram files are ignored by `.gitignore` and should not be pushed.
The generated benchmark CSV file is also ignored.

### Optional DOT Visualization

`./testBdd` writes a small `*BMD` DOT graph for 4-bit multiplication:

```sh
./testBdd
dot -Tpng bmd_multiply_4.dot -o bmd_multiply_4.png
```

The DOT graph labels the root weight and the `lo` / `hi` moment edges.  The
generated `.dot` and `.png` files are ignored by `.gitignore`.

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
- Exhaustive correctness checks for small arithmetic and Boolean cases.
- CSV benchmark result export for post-processing.
- DOT graph output for visualizing small `*BMD` examples.
- Restricted Verilog arithmetic input for simple word-level `+`, `-`, and `*`
  specifications.
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
  It also writes a small `bmd_multiply_4.dot` visualization example.

- `examples/add4.v`, `examples/sub4.v`, `examples/mul4.v`  
  Restricted Verilog arithmetic examples accepted by the CLI parser.

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
- DOT output is intended for small examples and demonstrations.  Large
  arithmetic graphs may still be difficult to inspect visually.
- Restricted Verilog input supports only simple word-level arithmetic
  assignments.  It is not a replacement for Yosys, ABC, or a complete Verilog
  parser.

## Experimental Results

The latest benchmark run passed all sample correctness checks.

The benchmark contains two categories:

- `*BMD` word-level arithmetic benchmarks:
  - `encode`: unsigned integer word representation
  - `add`: word-level addition `X + Y`
- `subtract`: word-level subtraction `X - Y`
  - `multiply`: word-level multiplication `X * Y`
- RicBDD BDD bit-level benchmarks:
  - `equality`: Boolean comparator `X == Y`
  - `add`: ripple-carry adder represented by output bits
  - `multiply`: array multiplier represented by output bits

The `encode` benchmark is included only for `*BMD` because it measures compact
word-level integer representation.  It is not a Boolean equality function and
should not be interpreted as corresponding to BDD `equality`.

Representative `*BMD` word-level results:

```text
Engine  Circuit       Bits       Nodes   Memory(est)     Seconds  Check
------------------------------------------------------------------------
*BMD    encode           4           5           240    0.000026  pass
*BMD    add              4           9           432    0.000011  pass
*BMD    multiply         4          15           720    0.000009  pass
*BMD    encode           8           9           432    0.000011  pass
*BMD    add              8          17           816    0.000021  pass
*BMD    subtract         8          17           816    0.000020  pass
*BMD    multiply         8          31          1488    0.000035  pass
*BMD    encode          12          13           624    0.000016  pass
*BMD    add             12          25          1200    0.000058  pass
*BMD    multiply        12          47          2256    0.000085  pass
*BMD    encode          16          17           816    0.000034  pass
*BMD    add             16          33          1584    0.000069  pass
*BMD    multiply        16          63          3024    0.000046  pass
*BMD    encode          24          25          1200    0.000091  pass
*BMD    add             24          49          2352    0.000176  pass
*BMD    encode          32          33          1584    0.000052  pass
*BMD    add             32          65          3120    0.000265  pass
```

Representative RicBDD BDD bit-level results:

```text
Engine  Circuit       Bits       Nodes   Memory(est)     Seconds  Check
------------------------------------------------------------------------
BDD     equality         4          23           552    0.000034  pass
BDD     add              4          39           936    0.000090  pass
BDD     equality         8          47          1128    0.000069  pass
BDD     add              8          83          1992    0.000160  pass
BDD     equality        12          71          1704    0.000079  pass
BDD     add             12         127          3048    0.000152  pass
BDD     equality        16          95          2280    0.000080  pass
BDD     add             16         171          4104    0.000403  pass
BDD     multiply         2          17           408    0.000044  pass
BDD     multiply         4         405          9720    0.000763  pass
BDD     multiply         6        5039        120936    0.008034  pass
BDD     multiply         8       55996       1343904    0.076185  pass
```

Additional correctness test:

```text
*BMD exhaustive 4-bit encode/add/multiply: pass
*BMD Boolean ops exhaustive NOT/AND/OR/XOR: pass
*BMD DOT example bmd_multiply_4.dot: written
Benchmark CSV benchmark_results.csv: written
```

Representative comparison summary:

```text
Comparison summary:
  8-bit multiply BDD nodes  : 55996
  8-bit multiply *BMD nodes : 31
  BDD/*BMD node ratio       : 1806.32x
```

Observations:

- `*BMD` represents unsigned word encoding linearly.
- `*BMD` addition also grows linearly in this benchmark.
- `*BMD` subtraction behaves similarly to addition and demonstrates support for
  integer-valued functions that may become negative.
- `*BMD` multiplication remains compact when constructed at the word level.
- RicBDD's BDD engine handles equality and addition well with interleaved
  variable ordering.
- Bit-level BDD multiplication grows much faster, reaching 55,996 nodes for
  only 8-bit multiplication.
- Runtime values may vary between runs, especially on a Raspberry Pi.  Node
  count and estimated memory are the primary comparison metrics.

These results are consistent with the motivation of `*BMD`: BDDs are useful for
Boolean control logic, while `*BMDs` are more suitable for word-level arithmetic
circuits such as RTL adders and multipliers.

## Limitations and Future Work

- Add arbitrary precision integer support for edge weights.
- Add unit tests instead of relying only on the benchmark driver.
- Add BLIF/AIG/ISCAS benchmark parsing.
- Integrate with Yosys/ABC for full Verilog-to-netlist workflows.
- Add hierarchical arithmetic circuit verification, following the ACV-style
  methodology described in prior work.
- Package the implementation as a cleaner pull request to the upstream RicBDD
  repository.

## Demo Video

Demo video: https://youtu.be/ibkdTyuhnbs

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
