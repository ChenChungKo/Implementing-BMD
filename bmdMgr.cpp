/****************************************************************************
  FileName     [ bmdMgr.cpp ]
  PackageName  [ ]
  Synopsis     [ *BMD Manager functions ]
****************************************************************************/

#include <cstdlib>
#include <cassert>
#include "bmdNode.h"
#include "bmdMgr.h"

using namespace std;

static long long
bmdAbs(long long v)
{
   return (v < 0)? -v: v;
}

static long long
bmdGcd(long long a, long long b)
{
   a = bmdAbs(a);
   b = bmdAbs(b);
   while (b != 0) {
      long long t = a % b;
      a = b;
      b = t;
   }
   return a;
}

static size_t
bmdMix(size_t h, size_t v)
{
   return h * 1315423911u + v + (h << 5) + (h >> 2);
}

static size_t
bmdPtr(const BmdNode& n)
{
   return n.getNodeId();
}

static bool
bmdNodeLess(const BmdNode& a, const BmdNode& b)
{
   if (a.getNodeId() != b.getNodeId()) return a.getNodeId() < b.getNodeId();
   return a() < b();
}

//----------------------------------------------------------------------
//    BmdHashKey / BmdCacheKey
//----------------------------------------------------------------------
size_t
BmdHashKey::operator() () const
{
   size_t h = 0;
   h = bmdMix(h, size_t(_loW));
   h = bmdMix(h, _lo);
   h = bmdMix(h, size_t(_hiW));
   h = bmdMix(h, _hi);
   h = bmdMix(h, _level);
   return h;
}

BmdCacheKey::BmdCacheKey(char op, const BmdNode& n1, const BmdNode& n2)
{
   _op = op;
   _w1 = n1();
   _n1 = bmdPtr(n1);
   _w2 = n2();
   _n2 = bmdPtr(n2);
}

size_t
BmdCacheKey::operator() () const
{
   size_t h = _op;
   h = bmdMix(h, size_t(_w1));
   h = bmdMix(h, _n1);
   h = bmdMix(h, size_t(_w2));
   h = bmdMix(h, _n2);
   return h;
}

//----------------------------------------------------------------------
//    class BmdMgr
//----------------------------------------------------------------------
void
BmdMgr::init(size_t nin, size_t h, size_t c)
{
   reset();
   _uniqueTable.init(h);
   _computedTable.init(c);
   _nextUid = 0;

   BmdNode::setBmdMgr(this);
   BmdNodeInt::_terminal = uniquify(0, BmdNode(), BmdNode());
   BmdNode::_zero = BmdNode(0, BmdNodeInt::_terminal);
   BmdNode::_one = BmdNode(1, BmdNodeInt::_terminal);

   _supports.reserve(nin + 1);
   _supports.push_back(BmdNode::_one);
   for (size_t i = 1; i <= nin; ++i)
      _supports.push_back(makeBranch(i, constant(0), constant(1)));
}

void
BmdMgr::restart()
{
   size_t nin = _supports.size() - 1;
   size_t h   = _uniqueTable.numBuckets();
   size_t c   = _computedTable.size();
   init(nin, h, c);
}

void
BmdMgr::reset()
{
   _supports.clear();
   BmdHash::iterator bi = _uniqueTable.begin();
   for (; bi != _uniqueTable.end(); ++bi)
      delete (*bi).second;
   _uniqueTable.reset();
   _computedTable.reset();
   _nextUid = 0;
}

BmdNode
BmdMgr::constant(long long v) const
{
   return BmdNode(v, BmdNodeInt::_terminal);
}

BmdNode
BmdMgr::applyWeight(long long w, const BmdNode& n) const
{
   if (w == 0 || n._weight == 0)
      return constant(0);
   return BmdNode(w * n._weight, n._node);
}

BmdNode
BmdMgr::makeBranch(unsigned level, BmdNode lo, BmdNode hi)
{
   // Positive Davio form: f = lo + x * hi.  A zero linear moment
   // makes the function independent of this variable.
   if (hi._weight == 0)
      return lo;

   long long w = bmdGcd(lo._weight, hi._weight);
   if (w == 0) w = 1;
   if (lo._weight < 0 || (lo._weight == 0 && hi._weight < 0))
      w = -w;

   lo._weight /= w;
   hi._weight /= w;

   BmdNodeInt* ni = uniquify(level, lo, hi);
   return BmdNode(w, ni);
}

BmdNodeInt*
BmdMgr::uniquify(unsigned level, const BmdNode& lo, const BmdNode& hi)
{
   BmdNodeInt* n = 0;
   BmdHashKey k(lo._weight, bmdPtr(lo), hi._weight, bmdPtr(hi), level);
   if (!_uniqueTable.check(k, n)) {
      n = new BmdNodeInt(level, lo, hi, _nextUid++);
      _uniqueTable.forceInsert(k, n);
   }
   return n;
}

BmdNode
BmdMgr::simpleMoment(const BmdNode& n, unsigned level, bool linear) const
{
   if (n.isTerminal() || n.getLevel() != level)
      return linear? constant(0): n;
   return applyWeight(n._weight, linear? n._node->getHi(): n._node->getLo());
}

BmdNode
BmdMgr::plusApply(BmdNode f, BmdNode g)
{
   if (f._weight == 0) return g;
   if (g._weight == 0) return f;
   if (f._node == g._node)
      return applyWeight(f._weight + g._weight, BmdNode(1, f._node));

   if (bmdNodeLess(g, f)) {
      BmdNode tmp = f;
      f = g;
      g = tmp;
   }

   BmdCacheKey k('+', f, g);
   BmdNode ret;
   if (_computedTable.read(k, ret))
      return ret;

   unsigned level = (f.getLevel() > g.getLevel())? f.getLevel(): g.getLevel();
   BmdNode lo = plusApply(simpleMoment(f, level, false),
                          simpleMoment(g, level, false));
   BmdNode hi = plusApply(simpleMoment(f, level, true),
                          simpleMoment(g, level, true));
   ret = makeBranch(level, lo, hi);
   _computedTable.write(k, ret);
   return ret;
}

BmdNode
BmdMgr::subtractApply(const BmdNode& f, const BmdNode& g)
{
   return plusApply(f, applyWeight(-1, g));
}

BmdNode
BmdMgr::multApply(BmdNode f, BmdNode g)
{
   if (f._weight == 0 || g._weight == 0)
      return constant(0);
   if (f.isTerminal())
      return applyWeight(f._weight, g);
   if (g.isTerminal())
      return applyWeight(g._weight, f);

   if (bmdNodeLess(g, f)) {
      BmdNode tmp = f;
      f = g;
      g = tmp;
   }

   BmdCacheKey k('*', f, g);
   BmdNode ret;
   if (_computedTable.read(k, ret))
      return ret;

   unsigned level = (f.getLevel() > g.getLevel())? f.getLevel(): g.getLevel();
   BmdNode fLo = simpleMoment(f, level, false);
   BmdNode fHi = simpleMoment(f, level, true);
   BmdNode gLo = simpleMoment(g, level, false);
   BmdNode gHi = simpleMoment(g, level, true);

   BmdNode lo = multApply(fLo, gLo);
   BmdNode hi = plusApply(multApply(fHi, gLo),
                          plusApply(multApply(fLo, gHi),
                                    multApply(fHi, gHi)));

   ret = makeBranch(level, lo, hi);
   _computedTable.write(k, ret);
   return ret;
}
