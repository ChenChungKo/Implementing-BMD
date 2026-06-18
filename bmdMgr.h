/****************************************************************************
  FileName     [ bmdMgr.h ]
  PackageName  [ ]
  Synopsis     [ Define *BMD Manager ]
****************************************************************************/

#ifndef BMD_MGR_H
#define BMD_MGR_H

#include <map>
#include <vector>
#include "myHash.h"
#include "bmdNode.h"

using namespace std;

class BmdHashKey
{
public:
   BmdHashKey() : _loW(0), _lo(0), _hiW(0), _hi(0), _level(0) {}
   BmdHashKey(long long lw, size_t lo, long long hw, size_t hi, unsigned l)
   : _loW(lw), _lo(lo), _hiW(hw), _hi(hi), _level(l) {}

   size_t operator() () const;
   bool operator == (const BmdHashKey& k) const {
      return (_loW == k._loW) && (_lo == k._lo) &&
             (_hiW == k._hiW) && (_hi == k._hi) && (_level == k._level);
   }

private:
   long long   _loW;
   size_t      _lo;
   long long   _hiW;
   size_t      _hi;
   unsigned    _level;
};

class BmdCacheKey
{
public:
   BmdCacheKey()
   : _op(0), _w1(0), _n1(0), _w2(0), _n2(0) {}
   BmdCacheKey(char op, const BmdNode& n1, const BmdNode& n2);

   size_t operator() () const;
   bool operator == (const BmdCacheKey& k) const {
      return (_op == k._op) && (_w1 == k._w1) && (_n1 == k._n1) &&
             (_w2 == k._w2) && (_n2 == k._n2);
   }

private:
   char       _op;
   long long  _w1;
   size_t     _n1;
   long long  _w2;
   size_t     _n2;
};

class BmdMgr
{
typedef Hash<BmdHashKey, BmdNodeInt*> BmdHash;
typedef Cache<BmdCacheKey, BmdNode>   BmdCache;

public:
   BmdMgr(size_t nin = 64, size_t h = 8009, size_t c = 30011)
   { init(nin, h, c); }
   ~BmdMgr() { reset(); }

   void init(size_t nin, size_t h, size_t c);
   void restart();

   BmdNode constant(long long v) const;
   BmdNode getSupport(size_t i) const { return _supports[i]; }
   BmdNode applyWeight(long long w, const BmdNode& n) const;
   BmdNode makeBranch(unsigned level, BmdNode lo, BmdNode hi);

   BmdNode plusApply(BmdNode f, BmdNode g);
   BmdNode multApply(BmdNode f, BmdNode g);
   BmdNode subtractApply(const BmdNode& f, const BmdNode& g);

   long long evalCube(const BmdNode& node, const string& pattern) const {
      return node.evalCube(pattern); }
   bool drawBmd(const string& name, const BmdNode& node,
                const string& dotFile) const;
   size_t getNumNodes() const { return _uniqueTable.size(); }
   size_t getMemUsage() const { return _uniqueTable.size() * sizeof(BmdNodeInt); }

private:
   vector<BmdNode>  _supports;
   BmdHash          _uniqueTable;
   BmdCache         _computedTable;
   size_t           _nextUid;

   void reset();
   BmdNodeInt* uniquify(unsigned level, const BmdNode& lo, const BmdNode& hi);
   BmdNode simpleMoment(const BmdNode& n, unsigned level, bool linear) const;
};

#endif // BMD_MGR_H
