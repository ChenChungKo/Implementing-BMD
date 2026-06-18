/****************************************************************************
  FileName     [ bmdNode.cpp ]
  PackageName  [ ]
  Synopsis     [ Define *BMD Node member functions ]
****************************************************************************/

#include <sstream>
#include <cassert>
#include "bmdNode.h"
#include "bmdMgr.h"

using namespace std;

BmdMgr* BmdNode::_BmdMgr = 0;
BmdNodeInt* BmdNodeInt::_terminal = 0;
BmdNode BmdNode::_one;
BmdNode BmdNode::_zero;
bool BmdNode::_debugBmdAddr = false;

BmdNode::BmdNode() : _weight(0), _node(BmdNodeInt::_terminal)
{
}

BmdNode::BmdNode(long long w, BmdNodeInt* n) : _weight(w), _node(n)
{
}

BmdNode
BmdNode::operator + (const BmdNode& n) const
{
   return _BmdMgr->plusApply((*this), n);
}

BmdNode&
BmdNode::operator += (const BmdNode& n)
{
   (*this) = (*this) + n;
   return (*this);
}

BmdNode
BmdNode::operator - (const BmdNode& n) const
{
   return _BmdMgr->subtractApply((*this), n);
}

BmdNode&
BmdNode::operator -= (const BmdNode& n)
{
   (*this) = (*this) - n;
   return (*this);
}

BmdNode
BmdNode::operator - () const
{
   return _BmdMgr->applyWeight(-1, (*this));
}

BmdNode
BmdNode::operator * (const BmdNode& n) const
{
   return _BmdMgr->multApply((*this), n);
}

BmdNode&
BmdNode::operator *= (const BmdNode& n)
{
   (*this) = (*this) * n;
   return (*this);
}

BmdNode
BmdNode::operator ~ () const
{
   return BmdNode::_one - (*this);
}

BmdNode
BmdNode::operator & (const BmdNode& n) const
{
   return (*this) * n;
}

BmdNode
BmdNode::operator | (const BmdNode& n) const
{
   return (*this) + n - ((*this) * n);
}

BmdNode
BmdNode::operator ^ (const BmdNode& n) const
{
   return (*this) + n - _BmdMgr->applyWeight(2, ((*this) * n));
}

unsigned
BmdNode::getLevel() const
{
   return isTerminal()? 0: _node->getLevel();
}

bool
BmdNode::isTerminal() const
{
   return (_node == BmdNodeInt::_terminal);
}

long long
BmdNode::evalCube(const string& pattern) const
{
   return evalCubeRecur(pattern);
}

long long
BmdNode::evalCubeRecur(const string& pattern) const
{
   if (isTerminal())
      return _weight;

   unsigned level = getLevel();
   assert(level > 0 && level <= pattern.size());

   long long lo = _node->getLo().evalCubeRecur(pattern);
   long long hi = _node->getHi().evalCubeRecur(pattern);
   long long x = (pattern[level - 1] == '1')? 1: 0;
   return _weight * (lo + x * hi);
}

size_t
BmdNode::countNode() const
{
   map<const BmdNodeInt*, bool> visited;
   countNodeRecur(visited);
   return visited.size();
}

void
BmdNode::countNodeRecur(map<const BmdNodeInt*, bool>& visited) const
{
   if (visited[_node]) return;
   visited[_node] = true;
   if (!isTerminal()) {
      _node->getLo().countNodeRecur(visited);
      _node->getHi().countNodeRecur(visited);
   }
}

string
BmdNode::getLabel() const
{
   stringstream s;
   if (isTerminal())
      s << "T";
   else
      s << "x" << getLevel() << "_" << _node->getUid();
   return s.str();
}

ostream&
operator << (ostream& os, const BmdNode& n)
{
   map<const BmdNodeInt*, bool> visited;
   n.print(os, 0, visited);
   os << endl << endl << "==> Total #BmdNodes : " << visited.size() << endl;
   return os;
}

void
BmdNode::print(ostream& os, size_t indent,
               map<const BmdNodeInt*, bool>& visited) const
{
   for (size_t i = 0; i < indent; ++i)
      os << ' ';

   os << "<" << _weight << ", ";
   if (isTerminal()) {
      os << "T>";
      visited[_node] = true;
      return;
   }

   os << "x" << getLevel();
   if (_debugBmdAddr)
      os << "@" << _node;
   os << ">";

   if (visited[_node]) {
      os << " (*)";
      return;
   }
   visited[_node] = true;

   os << endl;
   for (size_t i = 0; i < indent + 2; ++i) os << ' ';
   os << "lo: ";
   _node->getLo().print(os, indent + 4, visited);
   os << endl;
   for (size_t i = 0; i < indent + 2; ++i) os << ' ';
   os << "hi: ";
   _node->getHi().print(os, indent + 4, visited);
}

void
BmdNode::drawBmd(const string& name, ofstream& ofile) const
{
   map<const BmdNodeInt*, bool> visited;

   ofile << "digraph {" << endl;
   ofile << "   rankdir = TB;" << endl;
   ofile << "   node [shape = ellipse];" << endl;
   ofile << "   \"" << name << "\" [shape = plaintext];" << endl;
   ofile << "   \"T\" [shape = box, label = \"T\"];" << endl;
   ofile << "   \"" << name << "\" -> \"" << getLabel()
         << "\" [label = \"w=" << _weight << "\"];" << endl;

   drawBmdRecur(ofile, visited);

   ofile << "}" << endl;
}

void
BmdNode::drawBmdRecur(ofstream& ofile,
                      map<const BmdNodeInt*, bool>& visited) const
{
   if (isTerminal()) return;
   if (visited[_node]) return;
   visited[_node] = true;

   string thisLabel = getLabel();
   BmdNode lo = _node->getLo();
   BmdNode hi = _node->getHi();

   ofile << "   \"" << thisLabel << "\" [label = \"x"
         << getLevel() << "\\nuid=" << _node->getUid() << "\"];" << endl;

   ofile << "   \"" << thisLabel << "\" -> \"" << lo.getLabel()
         << "\" [style = dashed, label = \"lo w=" << lo._weight << "\"];" << endl;
   ofile << "   \"" << thisLabel << "\" -> \"" << hi.getLabel()
         << "\" [style = solid, label = \"hi w=" << hi._weight << "\"];" << endl;

   lo.drawBmdRecur(ofile, visited);
   hi.drawBmdRecur(ofile, visited);
}

BmdNodeInt::BmdNodeInt()
: _level(0), _lo(), _hi(), _uid(0)
{
}

BmdNodeInt::BmdNodeInt(unsigned l, const BmdNode& lo,
                       const BmdNode& hi, size_t uid)
: _level(l), _lo(lo), _hi(hi), _uid(uid)
{
}
