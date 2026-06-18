/****************************************************************************
  FileName     [ testBdd.cpp ]
  PackageName  [ ]
  Synopsis     [ Define main() ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2005-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/
#include <iostream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include "bddNode.h"
#include "bddMgr.h"
#include "bmdNode.h"
#include "bmdMgr.h"

using namespace std;

struct BenchRow
{
   string    engine;
   string    circuit;
   unsigned  bits;
   size_t    nodes;
   size_t    memory;
   double    seconds;
   string    check;
};

static BmdNode
buildBmdUnsigned(BmdMgr& bm, bool isY, unsigned bits)
{
   BmdNode f = bm.constant(0);
   for (unsigned i = 0; i < bits; ++i)
      f += bm.applyWeight(1LL << i, bm.getSupport(2 * i + (isY? 2: 1)));
   return f;
}

static vector<BddNode>
zeroBits(unsigned n)
{
   vector<BddNode> v(n, BddNode::_zero);
   return v;
}

static vector<BddNode>
addBits(const vector<BddNode>& a, const vector<BddNode>& b, unsigned width)
{
   vector<BddNode> s(width, BddNode::_zero);
   BddNode carry = BddNode::_zero;
   for (unsigned i = 0; i < width; ++i) {
      BddNode ai = (i < a.size())? a[i]: BddNode::_zero;
      BddNode bi = (i < b.size())? b[i]: BddNode::_zero;
      s[i] = ai ^ bi ^ carry;
      carry = (ai & bi) | (ai & carry) | (bi & carry);
   }
   return s;
}

static vector<BddNode>
multBits(const vector<BddNode>& x, const vector<BddNode>& y)
{
   unsigned n = x.size();
   unsigned width = 2 * n;
   vector<BddNode> product = zeroBits(width);

   for (unsigned i = 0; i < n; ++i) {
      vector<BddNode> partial = zeroBits(width);
      for (unsigned j = 0; j < n; ++j)
         partial[i + j] = x[j] & y[i];
      product = addBits(product, partial, width);
   }
   return product;
}

static vector<BddNode>
buildBddWord(BddMgr& bm, bool isY, unsigned bits)
{
   vector<BddNode> word(bits);
   for (unsigned i = 0; i < bits; ++i)
      word[i] = bm.getSupport(2 * i + (isY? 2: 1));
   return word;
}

static string
makePattern(unsigned bits, unsigned long long x, unsigned long long y)
{
   string p(2 * bits, '0');
   for (unsigned i = 0; i < bits; ++i) {
      if ((x >> i) & 1ULL) p[2 * i] = '1';
      if ((y >> i) & 1ULL) p[2 * i + 1] = '1';
   }
   return p;
}

static BenchRow
benchBmd(const string& circuit, unsigned bits)
{
   BmdMgr bm(2 * bits + 1, 20011, 80021);
   clock_t begin = clock();
   BmdNode x = buildBmdUnsigned(bm, false, bits);
   BmdNode y = buildBmdUnsigned(bm, true, bits);
   BmdNode f;

   if (circuit == "encode")
      f = x;
   else if (circuit == "add")
      f = x + y;
   else if (circuit == "subtract")
      f = x - y;
   else
      f = x * y;

   double sec = double(clock() - begin) / CLOCKS_PER_SEC;

   unsigned long long xv = (bits > 4)? 13: 3;
   unsigned long long yv = (bits > 4)? 7: 2;
   string pattern = makePattern(bits, xv, yv);
   long long expect = 0;
   if (circuit == "encode") expect = xv;
   else if (circuit == "add") expect = xv + yv;
   else if (circuit == "subtract") expect = (long long)xv - (long long)yv;
   else expect = xv * yv;

   string check = (bm.evalCube(f, pattern) == expect)? "pass": "fail";
   BenchRow row = { "*BMD", circuit, bits, f.countNode(),
                    f.countNode() * sizeof(BmdNodeInt), sec, check };
   return row;
}

static BenchRow
benchBdd(const string& circuit, unsigned bits)
{
   // RicBDD keeps terminal handles in static BddNode members.  Keep each
   // benchmark manager alive until program exit to avoid dangling terminals.
   BddMgr* bmp = new BddMgr(2 * bits + 1, 20011, 80021);
   BddMgr& bm = *bmp;
   clock_t begin = clock();
   vector<BddNode> x = buildBddWord(bm, false, bits);
   vector<BddNode> y = buildBddWord(bm, true, bits);
   vector<BddNode> out;
   BddNode eq = BddNode::_zero;

   if (circuit == "equality") {
      eq = BddNode::_one;
      for (unsigned i = 0; i < bits; ++i)
         eq &= ~(x[i] ^ y[i]);
   }
   else if (circuit == "add") {
      out = addBits(x, y, bits + 1);
   }
   else {
      out = multBits(x, y);
   }

   double sec = double(clock() - begin) / CLOCKS_PER_SEC;

   unsigned long long xv = (bits > 4)? 13: 3;
   unsigned long long yv = (bits > 4)? 7: 2;
   string pattern = makePattern(bits, xv, yv);
   string check = "pass";
   if (circuit == "equality") {
      int got = bm.evalCube(eq, pattern);
      check = (got == int(xv == yv))? "pass": "fail";
   }
   else {
      unsigned long long got = 0;
      for (unsigned i = 0; i < out.size(); ++i)
         if (bm.evalCube(out[i], pattern) == 1)
            got |= (1ULL << i);
      unsigned long long expect = (circuit == "add")? xv + yv: xv * yv;
      check = (got == expect)? "pass": "fail";
   }

   BenchRow row = { "BDD", circuit, bits, bm.getNumNodes(),
                    bm.getMemUsage(), sec, check };
   return row;
}

static void
writeCsv(const vector<BenchRow>& rows, const string& fileName)
{
   ofstream ofile(fileName.c_str());
   if (!ofile) return;

   ofile << "engine,circuit,bits,nodes,memory_est,seconds,check" << endl;
   for (size_t i = 0; i < rows.size(); ++i) {
      ofile << rows[i].engine << ','
            << rows[i].circuit << ','
            << rows[i].bits << ','
            << rows[i].nodes << ','
            << rows[i].memory << ','
            << fixed << setprecision(6) << rows[i].seconds << ','
            << rows[i].check << endl;
   }
}

static bool
exhaustiveBmd(unsigned bits)
{
   BmdMgr bm(2 * bits + 1, 20011, 80021);
   BmdNode x = buildBmdUnsigned(bm, false, bits);
   BmdNode y = buildBmdUnsigned(bm, true, bits);
   BmdNode sum = x + y;
   BmdNode product = x * y;

   unsigned long long limit = 1ULL << bits;
   for (unsigned long long xv = 0; xv < limit; ++xv) {
      for (unsigned long long yv = 0; yv < limit; ++yv) {
         string pattern = makePattern(bits, xv, yv);
         if (bm.evalCube(x, pattern) != (long long)xv)
            return false;
         if (bm.evalCube(sum, pattern) != (long long)(xv + yv))
            return false;
         if (bm.evalCube(product, pattern) != (long long)(xv * yv))
            return false;
      }
   }
   return true;
}

static bool
exhaustiveBmdBoolean()
{
   BmdMgr bm(2, 20011, 80021);
   BmdNode a = bm.getSupport(1);
   BmdNode b = bm.getSupport(2);
   BmdNode notA = ~a;
   BmdNode andAB = a & b;
   BmdNode orAB = a | b;
   BmdNode xorAB = a ^ b;

   for (unsigned av = 0; av <= 1; ++av) {
      for (unsigned bv = 0; bv <= 1; ++bv) {
         string pattern = makePattern(1, av, bv);
         if (bm.evalCube(notA, pattern) != (long long)(!av))
            return false;
         if (bm.evalCube(andAB, pattern) != (long long)(av & bv))
            return false;
         if (bm.evalCube(orAB, pattern) != (long long)(av | bv))
            return false;
         if (bm.evalCube(xorAB, pattern) != (long long)(av ^ bv))
            return false;
      }
   }
   return true;
}

static bool
writeBmdDotExample()
{
   BmdMgr bm(9, 20011, 80021);
   BmdNode x = buildBmdUnsigned(bm, false, 4);
   BmdNode y = buildBmdUnsigned(bm, true, 4);
   BmdNode product = x * y;
   return bm.drawBmd("mul4", product, "bmd_multiply_4.dot");
}

static void
printRow(const BenchRow& r)
{
   cout << left << setw(8) << r.engine
        << setw(12) << r.circuit
        << right << setw(6) << r.bits
        << setw(12) << r.nodes
        << setw(14) << r.memory
        << setw(12) << fixed << setprecision(6) << r.seconds
        << "  " << r.check << endl;
}

static size_t
findNodes(const vector<BenchRow>& rows, const string& engine,
          const string& circuit, unsigned bits)
{
   for (size_t i = 0; i < rows.size(); ++i)
      if (rows[i].engine == engine &&
          rows[i].circuit == circuit &&
          rows[i].bits == bits)
         return rows[i].nodes;
   return 0;
}

int
main()
{
   vector<BenchRow> rows;

   cout << "BMDImpt benchmark on RicBDD infrastructure" << endl;
   cout << "Positive Davio arithmetic form: f = f0 + x * (f1 - f0)" << endl;
   cout << endl;
   cout << left << setw(8) << "Engine"
        << setw(12) << "Circuit"
        << right << setw(6) << "Bits"
        << setw(12) << "Nodes"
        << setw(14) << "Memory(est)"
        << setw(12) << "Seconds"
        << "  Check" << endl;
   cout << string(72, '-') << endl;

   unsigned bmdBits[] = { 4, 8, 12, 16, 24, 32 };
   for (unsigned i = 0; i < sizeof(bmdBits) / sizeof(unsigned); ++i) {
      rows.push_back(benchBmd("encode", bmdBits[i]));
      printRow(rows.back());
      rows.push_back(benchBmd("add", bmdBits[i]));
      printRow(rows.back());
      rows.push_back(benchBmd("subtract", bmdBits[i]));
      printRow(rows.back());
      if (bmdBits[i] <= 16) {
         rows.push_back(benchBmd("multiply", bmdBits[i]));
         printRow(rows.back());
      }
   }

   unsigned bddAddBits[] = { 4, 8, 12, 16 };
   for (unsigned i = 0; i < sizeof(bddAddBits) / sizeof(unsigned); ++i) {
      rows.push_back(benchBdd("equality", bddAddBits[i]));
      printRow(rows.back());
      rows.push_back(benchBdd("add", bddAddBits[i]));
      printRow(rows.back());
   }

   unsigned bddMulBits[] = { 2, 4, 6, 8 };
   for (unsigned i = 0; i < sizeof(bddMulBits) / sizeof(unsigned); ++i) {
      rows.push_back(benchBdd("multiply", bddMulBits[i]));
      printRow(rows.back());
   }

   cout << endl;
   cout << "*BMD exhaustive 4-bit encode/add/multiply: "
        << (exhaustiveBmd(4)? "pass": "fail") << endl;
   cout << "*BMD Boolean ops exhaustive NOT/AND/OR/XOR: "
        << (exhaustiveBmdBoolean()? "pass": "fail") << endl;
   cout << "*BMD DOT example bmd_multiply_4.dot: "
        << (writeBmdDotExample()? "written": "failed") << endl;

   writeCsv(rows, "benchmark_results.csv");
   cout << "Benchmark CSV benchmark_results.csv: written" << endl;

   size_t bmdMul8 = findNodes(rows, "*BMD", "multiply", 8);
   size_t bddMul8 = findNodes(rows, "BDD", "multiply", 8);
   if (bmdMul8 != 0 && bddMul8 != 0) {
      cout << endl;
      cout << "Comparison summary:" << endl;
      cout << "  8-bit multiply BDD nodes  : " << bddMul8 << endl;
      cout << "  8-bit multiply *BMD nodes : " << bmdMul8 << endl;
      cout << "  BDD/*BMD node ratio       : "
           << fixed << setprecision(2)
           << (double(bddMul8) / double(bmdMul8)) << "x" << endl;
   }

   return 0;
}

