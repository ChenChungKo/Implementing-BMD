/****************************************************************************
  FileName     [ bmdNode.h ]
  PackageName  [ ]
  Synopsis     [ Define basic *BMD Node data structures ]
****************************************************************************/

#ifndef BMD_NODE_H
#define BMD_NODE_H

#include <map>
#include <iostream>
#include <fstream>

using namespace std;

class BmdMgr;
class BmdNodeInt;

class BmdNode
{
public:
   static BmdNode          _one;
   static BmdNode          _zero;
   static bool             _debugBmdAddr;

   BmdNode();
   BmdNode(long long w, BmdNodeInt* n);

   long long operator () () const { return _weight; }
   size_t getNodeId() const { return size_t(_node); }
   bool operator == (const BmdNode& n) const {
      return (_weight == n._weight) && (_node == n._node); }
   bool operator != (const BmdNode& n) const { return !((*this) == n); }

   BmdNode operator + (const BmdNode& n) const;
   BmdNode& operator += (const BmdNode& n);
   BmdNode operator - (const BmdNode& n) const;
   BmdNode& operator -= (const BmdNode& n);
   BmdNode operator - () const;
   BmdNode operator * (const BmdNode& n) const;
   BmdNode& operator *= (const BmdNode& n);

   // Boolean functions are represented as arithmetic 0/1 functions.
   BmdNode operator ~ () const;
   BmdNode operator & (const BmdNode& n) const;
   BmdNode operator | (const BmdNode& n) const;
   BmdNode operator ^ (const BmdNode& n) const;

   unsigned getLevel() const;
   long long evalCube(const string& pattern) const;
   size_t countNode() const;
   string getLabel() const;
   void drawBmd(const string& name, ofstream& ofile) const;

   friend ostream& operator << (ostream& os, const BmdNode& n);

   static void setBmdMgr(BmdMgr* m) { _BmdMgr = m; }

private:
   long long               _weight;
   BmdNodeInt*             _node;

   static BmdMgr*          _BmdMgr;

   bool isTerminal() const;
   long long evalCubeRecur(const string& pattern) const;
   void countNodeRecur(map<const BmdNodeInt*, bool>& visited) const;
   void print(ostream&, size_t, map<const BmdNodeInt*, bool>&) const;
   void drawBmdRecur(ofstream&, map<const BmdNodeInt*, bool>&) const;

   friend class BmdMgr;
   friend class BmdNodeInt;
};

class BmdNodeInt
{
   friend class BmdNode;
   friend class BmdMgr;

   BmdNodeInt();
   BmdNodeInt(unsigned l, const BmdNode& lo, const BmdNode& hi, size_t uid);

   unsigned getLevel() const { return _level; }
   const BmdNode& getLo() const { return _lo; }
   const BmdNode& getHi() const { return _hi; }
   size_t getUid() const { return _uid; }

   unsigned             _level;
   BmdNode              _lo;
   BmdNode              _hi;
   size_t               _uid;

   static BmdNodeInt*   _terminal;
};

#endif // BMD_NODE_H
