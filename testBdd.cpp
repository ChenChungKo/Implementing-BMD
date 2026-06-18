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
   else
      f = x * y;

   double sec = double(clock() - begin) / CLOCKS_PER_SEC;

   unsigned long long xv = (bits > 4)? 13: 3;
   unsigned long long yv = (bits > 4)? 7: 2;
   string pattern = makePattern(bits, xv, yv);
   long long expect = 0;
   if (circuit == "encode") expect = xv;
   else if (circuit == "add") expect = xv + yv;
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

int
main()
{
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
      printRow(benchBmd("encode", bmdBits[i]));
      printRow(benchBmd("add", bmdBits[i]));
      if (bmdBits[i] <= 16)
         printRow(benchBmd("multiply", bmdBits[i]));
   }

   unsigned bddAddBits[] = { 4, 8, 12, 16 };
   for (unsigned i = 0; i < sizeof(bddAddBits) / sizeof(unsigned); ++i) {
      printRow(benchBdd("equality", bddAddBits[i]));
      printRow(benchBdd("add", bddAddBits[i]));
   }

   unsigned bddMulBits[] = { 2, 4, 6, 8 };
   for (unsigned i = 0; i < sizeof(bddMulBits) / sizeof(unsigned); ++i)
      printRow(benchBdd("multiply", bddMulBits[i]));

   return 0;
}

