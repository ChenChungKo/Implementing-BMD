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
#include <sstream>
#include <cctype>
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

struct Options
{
   bool     all;
   string   engine;
   string   circuit;
   unsigned bits;
   string   csvFile;
   string   dotFile;
   string   verilogFile;
   bool     writeDot;

   Options()
   : all(true), engine(""), circuit(""), bits(0),
     csvFile("benchmark_results.csv"),
     dotFile("bmd_multiply_4.dot"), verilogFile(""), writeDot(true) {}
};

struct VerilogSpec
{
   unsigned bits;
   char     op;
   string   circuit;

   VerilogSpec() : bits(0), op(0), circuit("") {}
};

static bool parseUInt(const string& str, unsigned& value);

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

static bool
isBmdCircuit(const string& circuit)
{
   return circuit == "encode" || circuit == "add" ||
          circuit == "subtract" || circuit == "multiply";
}

static bool
isBddCircuit(const string& circuit)
{
   return circuit == "equality" || circuit == "add" ||
          circuit == "multiply";
}

static string
readFileText(const string& fileName)
{
   ifstream ifile(fileName.c_str());
   if (!ifile) return "";
   stringstream ss;
   ss << ifile.rdbuf();
   return ss.str();
}

static string
stripSpaces(const string& str)
{
   string out;
   for (size_t i = 0; i < str.size(); ++i)
      if (!isspace((unsigned char)str[i]))
         out += str[i];
   return out;
}

static bool
parseInputWidth(const string& text, const string& name, unsigned& bits)
{
   string key = "input[";
   size_t pos = 0;
   while ((pos = text.find(key, pos)) != string::npos) {
      size_t colon = text.find(':', pos + key.size());
      size_t close = text.find(']', colon);
      if (colon == string::npos || close == string::npos)
         return false;

      string highStr = text.substr(pos + key.size(), colon - (pos + key.size()));
      string lowStr = text.substr(colon + 1, close - colon - 1);
      unsigned high = 0, low = 0;
      if (!parseUInt(highStr, high) || !parseUInt(lowStr, low))
         return false;

      size_t namePos = close + 1;
      if (text.compare(namePos, name.size(), name) == 0) {
         if (low != 0 || high == 0)
            return false;
         bits = high + 1;
         return true;
      }
      ++pos;
   }
   return false;
}

static bool
parseRestrictedVerilog(const string& fileName, VerilogSpec& spec)
{
   string text = stripSpaces(readFileText(fileName));
   if (text.empty()) return false;

   unsigned xBits = 0, yBits = 0;
   if (!parseInputWidth(text, "x", xBits)) return false;
   if (!parseInputWidth(text, "y", yBits)) return false;
   if (xBits != yBits) return false;

   size_t assignPos = text.find("assign");
   size_t eqPos = text.find('=', assignPos);
   size_t semiPos = text.find(';', eqPos);
   if (assignPos == string::npos || eqPos == string::npos ||
       semiPos == string::npos)
      return false;

   string expr = text.substr(eqPos + 1, semiPos - eqPos - 1);
   if (expr == "x+y") {
      spec.circuit = "add";
      spec.op = '+';
   }
   else if (expr == "x-y") {
      spec.circuit = "subtract";
      spec.op = '-';
   }
   else if (expr == "x*y") {
      spec.circuit = "multiply";
      spec.op = '*';
   }
   else
      return false;

   spec.bits = xBits;
   return true;
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
writeBmdDotExample(const string& fileName)
{
   BmdMgr bm(9, 20011, 80021);
   BmdNode x = buildBmdUnsigned(bm, false, 4);
   BmdNode y = buildBmdUnsigned(bm, true, 4);
   BmdNode product = x * y;
   return bm.drawBmd("mul4", product, fileName);
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

static bool
parseUInt(const string& str, unsigned& value)
{
   stringstream ss(str);
   unsigned v;
   char extra;
   if (!(ss >> v)) return false;
   if (ss >> extra) return false;
   value = v;
   return true;
}

static void
printUsage(const char* argv0)
{
   cout << "Usage:" << endl;
   cout << "  " << argv0 << " [--all]" << endl;
   cout << "  " << argv0
        << " --engine <bmd|bdd> --circuit <name> --bits <n> [--csv <file>] [--dot <file>]" << endl;
   cout << "  " << argv0
        << " --verilog <restricted-arithmetic.v> [--csv <file>] [--dot <file>]" << endl;
   cout << endl;
   cout << "BMD circuits: encode, add, subtract, multiply" << endl;
   cout << "BDD circuits: equality, add, multiply" << endl;
   cout << "Restricted Verilog supports: assign z = x + y, x - y, or x * y" << endl;
   cout << endl;
   cout << "Examples:" << endl;
   cout << "  " << argv0 << " --engine bmd --circuit multiply --bits 16" << endl;
   cout << "  " << argv0 << " --engine bdd --circuit multiply --bits 8 --csv bdd_mul8.csv" << endl;
   cout << "  " << argv0 << " --verilog examples/mul4.v" << endl;
}

static bool
parseArgs(int argc, char** argv, Options& opt)
{
   for (int i = 1; i < argc; ++i) {
      string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
         printUsage(argv[0]);
         exit(0);
      }
      else if (arg == "--all") {
         opt.all = true;
      }
      else if (arg == "--engine" && i + 1 < argc) {
         opt.engine = argv[++i];
         opt.all = false;
      }
      else if (arg == "--circuit" && i + 1 < argc) {
         opt.circuit = argv[++i];
         opt.all = false;
      }
      else if (arg == "--bits" && i + 1 < argc) {
         if (!parseUInt(argv[++i], opt.bits))
            return false;
         opt.all = false;
      }
      else if (arg == "--csv" && i + 1 < argc) {
         opt.csvFile = argv[++i];
      }
      else if (arg == "--verilog" && i + 1 < argc) {
         opt.verilogFile = argv[++i];
         opt.engine = "bmd";
         opt.all = false;
      }
      else if (arg == "--dot" && i + 1 < argc) {
         opt.dotFile = argv[++i];
         opt.writeDot = true;
      }
      else if (arg == "--no-dot") {
         opt.writeDot = false;
      }
      else {
         cerr << "Unknown or incomplete option: " << arg << endl;
         return false;
      }
   }

   if (!opt.all) {
      if (opt.verilogFile != "") {
         VerilogSpec spec;
         if (!parseRestrictedVerilog(opt.verilogFile, spec)) {
            cerr << "Error: cannot parse restricted Verilog file: "
                 << opt.verilogFile << endl;
            return false;
         }
         opt.circuit = spec.circuit;
         opt.bits = spec.bits;
         return true;
      }
      if (opt.engine != "bmd" && opt.engine != "bdd") {
         cerr << "Error: --engine must be bmd or bdd" << endl;
         return false;
      }
      if (opt.bits == 0) {
         cerr << "Error: --bits must be a positive integer" << endl;
         return false;
      }
      if (opt.engine == "bmd" && !isBmdCircuit(opt.circuit)) {
         cerr << "Error: unsupported BMD circuit: " << opt.circuit << endl;
         return false;
      }
      if (opt.engine == "bdd" && !isBddCircuit(opt.circuit)) {
         cerr << "Error: unsupported BDD circuit: " << opt.circuit << endl;
         return false;
      }
   }

   return true;
}

int
main(int argc, char** argv)
{
   Options opt;
   if (!parseArgs(argc, argv, opt)) {
      printUsage(argv[0]);
      return 1;
   }

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

   if (opt.all) {
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
   }
   else {
      if (opt.engine == "bmd")
         rows.push_back(benchBmd(opt.circuit, opt.bits));
      else
         rows.push_back(benchBdd(opt.circuit, opt.bits));
      printRow(rows.back());
   }

   cout << endl;
   if (opt.all || opt.engine == "bmd") {
      cout << "*BMD exhaustive 4-bit encode/add/multiply: "
           << (exhaustiveBmd(4)? "pass": "fail") << endl;
      cout << "*BMD Boolean ops exhaustive NOT/AND/OR/XOR: "
           << (exhaustiveBmdBoolean()? "pass": "fail") << endl;
   }
   if (opt.writeDot) {
      cout << "*BMD DOT example " << opt.dotFile << ": "
           << (writeBmdDotExample(opt.dotFile)? "written": "failed") << endl;
   }

   writeCsv(rows, opt.csvFile);
   cout << "Benchmark CSV " << opt.csvFile << ": written" << endl;

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

