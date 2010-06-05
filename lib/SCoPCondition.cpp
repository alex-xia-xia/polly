//===---------- SCoPConditon.cpp  - Extract conditons in CFG ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implement a pass that extrct the conditon information of CFGs.
//
//===----------------------------------------------------------------------===//

#include "polly/SCoPCondition.h"
#include "llvm/Support/CFG.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/Support/ErrorHandling.h"
#define DEBUG_TYPE "polly-scop-cond"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace polly;

/// Some complexity sorting code from ScalarEvolution.cpp
namespace {
/// SCoPCndComplexityCompare - Return true if the complexity of the LHS is less
/// than the complexity of the RHS.  This comparator is used to canonicalize
/// expressions.
struct SCoPCndComplexityCompare {
  explicit SCoPCndComplexityCompare() {}
  bool operator()(const SCoPCnd *LHS, const SCoPCnd *RHS) const {
    if (LHS == RHS)
      return false;
    if (LHS->getSCoPCndType() != RHS->getSCoPCndType())
      return LHS->getSCoPCndType() < RHS->getSCoPCndType();

    // Now LHS and RHS have the same type
    // Check Branching condition.
    if (const SCoPBrCnd *LBr = dyn_cast<SCoPBrCnd>(LHS)) {
      const SCoPBrCnd *RBr = cast<SCoPBrCnd>(RHS);
      if (LBr->getCnd() != RBr->getCnd()) {
        // FIXME: How to compare the conditions?
        return LBr->getCnd() < RBr->getCnd();
      }
      // Branching with the same condition
      assert(LBr->getSide() != RBr->getSide()
        && "Why we get two same branching condition?");
      return LBr->getSide();
    }

    // Or condition
    if (const SCoPOrCnd *LOr = dyn_cast<SCoPOrCnd>(LHS)) {
      const SCoPOrCnd *ROr = dyn_cast<SCoPOrCnd>(RHS);
      return SortNAryCnd(LOr, ROr);
    }
    // And condition
    if (const SCoPAndCnd *LAnd = dyn_cast<SCoPAndCnd>(LHS)) {
      const SCoPAndCnd *RAnd = dyn_cast<SCoPAndCnd>(RHS);
      return SortNAryCnd(LAnd, RAnd);
    }

    llvm_unreachable("UnSupport SCoPCond kind!");
    return false;
  }

  template<class CndType>
  bool SortNAryCnd(const CndType *LHS, const CndType *RHS) const {
    for (unsigned i = 0, e = LHS->getNumOperands(); i != e; ++i) {
      if (i >= RHS->getNumOperands())
        return false;
      if (operator()(LHS->getOperand(i), RHS->getOperand(i)))
        return true;
      if (operator()(RHS->getOperand(i), LHS->getOperand(i)))
        return false;
    }
    return LHS->getNumOperands() < RHS->getNumOperands();
  }
};
} // end namespace

static void GroupByComplexity(SmallVectorImpl<const SCoPCnd*> &Ops) {
  if (Ops.size() < 2) return;  // Noop
  if (Ops.size() == 2) {
    // This is the common case, which also happens to be trivially simple.
    // Special case it.
    if (SCoPCndComplexityCompare()(Ops[1], Ops[0]))
      std::swap(Ops[0], Ops[1]);
    return;
  }

  // Do the rough sort by complexity.
  std::stable_sort(Ops.begin(), Ops.end(), SCoPCndComplexityCompare());

  // Now that we are sorted by complexity, group elements of the same
  // complexity.  Note that this is, at worst, N^2, but the vector is likely to
  // be extremely short in practice.  Note that we take this approach because we
  // do not want to depend on the addresses of the objects we are grouping.
  for (unsigned i = 0; i < Ops.size() - 1; ++i) {
    const SCoPCnd *S = Ops[i];
    unsigned Complexity = S->getSCoPCndType();
    // If there are any objects of the same complexity and same value as this
    // one, group them.
    unsigned j = i + 1;
    while(j < Ops.size() && Ops[j]->getSCoPCndType() == Complexity) {
      if (Ops[j] == S) {
        // Remove the duplicate condition, because A & A = A, A | A = A
        Ops.erase(Ops.begin() + j);
        // Do not increase j since we had removed something
        continue;
      }
      ++j;
    }
  }
}

//===----------------------------------------------------------------------===//
void SCoPBrCnd::print(raw_ostream &OS) const {
  if (!Cond.getInt())
    OS << "!";
  WriteAsOperand(OS, Cond.getPointer(), false);
}


//===----------------------------------------------------------------------===//
// Implementation of SCoPConditions

const SCoPCnd *SCoPCondition::getBrCnd(BranchInst *Br, bool Side) {
  // Return always true for unconditional branch.
  if (!Br->isConditional()) {
    assert(Side && "What is the false side of UnConditional Branch?");
    return getAlwaysTrue();
  }

  // Get the condition of Conditional Branch
  Value *Cnd = Br->getCondition();
  if (ConstantInt *ConstCnd = dyn_cast<ConstantInt>(Cnd)) {
    Cnd = 0;
    // Evaluate the constant condition.
    if (ConstCnd->isOne() == Side)
      return getAlwaysTrue();
    else
      return getAlwaysFalse();
  }
  // TODO: FoldingSet logic?
  SCoPCnd *C = new (SCoPCndAllocator) SCoPBrCnd(Cnd, Side);
  return C;
}

const SCoPCnd *SCoPCondition::getOrCnd(const SCoPCnd *LHS,
                                       const SCoPCnd *RHS) {
  SmallVector<const SCoPCnd*, 2> Ops;
  Ops.push_back(LHS);
  Ops.push_back(RHS);
  return getOrCnd(Ops);
}

const SCoPCnd *SCoPCondition::getOrCnd(SmallVectorImpl<const SCoPCnd *> &Ops) {
  DEBUG(
    for (unsigned i = 0, e = Ops.size(); i != e; ++i) {
      dbgs() << "Cnd in or cnd: ";
      Ops[i]->print(dbgs());
      dbgs() << "\n";
    }
  );
  // Handle the trival condition
  if (Ops.size() == 1)
    return Ops[0];

  // Sort Ops by similarity
  GroupByComplexity(Ops);
  unsigned idx = 0;
  // False || Whatever = Whatever
  if (isa<SCoPFalseCnd>(Ops[idx]))
    Ops.erase(Ops.begin());

  // True || Whatever = True
  if (isa<SCoPTrueCnd>(Ops[idx]))
    return getAlwaysTrue();

  // Find the first br condition
  for (unsigned op_size = Ops.size();
      idx < op_size &&  Ops[idx]->getSCoPCndType() < scopBrCnd; ++idx)
    ;

  // Could us find any reducible branch conditions?
  while (idx < Ops.size() - 1 && Ops[idx + 1]->getSCoPCndType() < scopSWCnd) {
    if (isOppBrCnd(cast<SCoPBrCnd>(Ops[idx]), cast<SCoPBrCnd>(Ops[idx+1]))) {
      // Erase both conditions
      Ops.erase(Ops.begin() + idx, Ops.begin() + idx + 2);
      // Do not increase idx;
      continue;
    }
    ++idx;
  }

  // Flatten the expression tree of Or Condition?
  for (unsigned i = 0, e = Ops.size(); i != e; ++i)
    // A || !A = True
    if (const SCoPOrCnd *Or = dyn_cast<SCoPOrCnd>(Ops[i])) {
      Ops.erase(Ops.begin() + i);
      Ops.append(Or->op_begin(), Or->op_end());
      return getOrCnd(Ops);
    }

  // ... How should us handle the And Condition?
  //

  // Dirty Hack:
  if (Ops.size() == 1)
    return Ops[0];


  // Create the condition
  const SCoPCnd **O = SCoPCndAllocator.Allocate<const SCoPCnd*>(Ops.size());
  std::uninitialized_copy(Ops.begin(), Ops.end(), O);
  const SCoPCnd *C = new (SCoPCndAllocator) SCoPOrCnd(O, Ops.size());
  return C;
}

const SCoPCnd *SCoPCondition::getAndCnd(const SCoPCnd *LHS,
                                       const SCoPCnd *RHS) {
  SmallVector<const SCoPCnd*, 2> Ops;
  Ops.push_back(LHS);
  Ops.push_back(RHS);
  return getAndCnd(Ops);
}


const SCoPCnd *SCoPCondition::getAndCnd(SmallVectorImpl<const SCoPCnd *> &Ops) {
  // Handle the trival condition
  if (Ops.size() == 1)
    return Ops[0];
  // Sort Ops by similarity
  GroupByComplexity(Ops);
  unsigned idx = 0;
  // False && Whatever = False
  if (isa<SCoPFalseCnd>(Ops[idx]))
    return getAlwaysFalse();

  // True && Whatever = Whatever
  if (isa<SCoPTrueCnd>(Ops[idx]))
    Ops.erase(Ops.begin());

  // Find the first br condition
  for (unsigned op_size = Ops.size();
    idx < op_size &&  Ops[idx]->getSCoPCndType() < scopBrCnd; ++idx)
    ;

  // Could us find any reducible branch conditions?
  while (idx < Ops.size() - 1 && Ops[idx + 1]->getSCoPCndType() < scopSWCnd) {
    // A && !A = False
    if (isOppBrCnd(cast<SCoPBrCnd>(Ops[idx]), cast<SCoPBrCnd>(Ops[idx+1])))
      return getAlwaysFalse();

    ++idx;
  }

  // Flatten the expression tree of and Condition?
  for (unsigned i = 0, e = Ops.size(); i != e; ++i)
    if (const SCoPAndCnd *Or = dyn_cast<SCoPAndCnd>(Ops[i])) {
      Ops.erase(Ops.begin() + i);
      Ops.append(Or->op_begin(), Or->op_end());
      return getAndCnd(Ops);
    }

  // Dirty Hack:
  if (Ops.size() == 1)
    return Ops[0];

  // ... How should us handle the And Condition?
  // Create the condition
  const SCoPCnd **O = SCoPCndAllocator.Allocate<const SCoPCnd*>(Ops.size());
  std::uninitialized_copy(Ops.begin(), Ops.end(), O);
  const SCoPCnd *C = new (SCoPCndAllocator) SCoPAndCnd(O, Ops.size());
  return C;
}

bool SCoPCondition::isOppBrCnd(const SCoPBrCnd *BrLHS,
                                               const SCoPBrCnd *BrRHS) const {
  // If both of them are according the same condition?
  if (BrLHS->getCnd() == BrRHS->getCnd())
    // If the two Conditions have difference side?
    if (BrLHS->getSide() != BrRHS->getSide())
      // The two conditions have the same cmp instruction, but branching to
      // difference side.
      return true;

  return false;
}


void SCoPCondition::visitBBWithCnd(BasicBlock *BB, const SCoPCnd *Cnd) {
  if (const SCoPCnd *C = getImmCndFor(BB)) {
    ImmCnd[BB] = getOrCnd(Cnd, C);
  } else
    ImmCnd[BB] = Cnd;
}

void SCoPCondition::applyBBWithCnd(BasicBlock *BB, BasicBlock *DomBB) {
  assert(ImmCnd.count(BB) && ImmCnd.count(DomBB)
    && "Not visit BB and DomBB yet?");

  ImmCnd[BB] = getAndCnd(ImmCnd[BB], ImmCnd[DomBB]);
}

void SCoPCondition::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.setPreservesAll();
}

void SCoPCondition::findAllPathForBB(BasicBlock *BB, DomTreeNode *Dst, DomTreeNode *Src,
                                     SmallVectorImpl<const SCoPCnd *> &Cnds) {
  // Reach Src
  if (Src == Dst) {
    const SCoPCnd *Res = getAndCnd(Cnds);
    visitBBWithCnd(BB, Res);
    DEBUG(dbgs() << Dst->getBlock()->getNameStr() << "\n");
    DEBUG(Res->print(dbgs()));
    DEBUG(dbgs() << "\n");
    return;
  }

  BasicBlock *DstBB = Dst->getBlock();
  for(pred_iterator I = pred_begin(DstBB), E = pred_end(DstBB); I != E; ++I) {
    DEBUG(dbgs() << Dst->getBlock()->getNameStr() << ", ");
    BasicBlock *PredBB = *I;
    // TODO: Do not visit Back-edge.
    if (DT->dominates(Dst, DT->getNode(PredBB))) {
      DEBUG(dbgs() << "is back edge!\n");
      continue;
    }

    BranchInst *Br = dyn_cast<BranchInst>(PredBB->getTerminator());
    // We are not support Switch at this moment
    if (!Br) return;
    // Add the condition for conditional branch
    if (Br->isConditional()) {
      const SCoPCnd *Cnd = getBrCnd(Br, (Br->getSuccessor(0) == DstBB));
      Cnds.push_back(Cnd);
    }
    findAllPathForBB(BB, DT->getNode(PredBB), Src, Cnds);
    // Discard the condition when we leave this bb.
    if (Br->isConditional())
      Cnds.pop_back();
  }
}

void SCoPCondition::visitBB(DomTreeNode *N) {
  assert(N && "Root can not be null!");

  typedef GraphTraits<DominatorTree*> GraphT;
  typedef GraphTraits<DominatorTree*>::ChildIteratorType ChildIteratorType;

  SmallVector<const SCoPCnd*, 8> Cnds;

  for (GraphTraits<DominatorTree*>::ChildIteratorType
      CI = GraphT::child_begin(N), CE = GraphT::child_end(N); CI != CE; ++CI) {
    DomTreeNode *Node = *CI;
    BasicBlock *BB = Node->getBlock();
    DEBUG(dbgs() << "{\n");
    findAllPathForBB(BB, Node, N, Cnds);
    assert(Cnds.empty() && "All conditions expect to be poped!");
    DEBUG(dbgs() << "}\n");
    //
    applyBBWithCnd(BB, N->getBlock());
    visitBB(Node);
  }
}

bool SCoPCondition::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTree>();
  DomTreeNode *Root = DT->getRootNode();
  visitBBWithCnd(Root->getBlock(), getAlwaysTrue());
  visitBB(Root);
  return false;
}

void SCoPCondition::print(raw_ostream &OS, const Module *) const {
  Function *F = DT->getRoot()->getParent();
  for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I) {
    BasicBlock *BB = I;
    CndMapTy::const_iterator at = ImmCnd.find(BB);
    // Unreachable block?
    if (at == ImmCnd.end())
      continue;

    OS << "Condition of BB: " << at->first->getName() << " is\n";
    at->second->print(OS.indent(2));
    OS << "\n";
  }
}


char SCoPCondition::ID = 0;

static RegisterPass<SCoPCondition>
X("polly-scop-condition", "Polly - Extract conditions Constraint for each BB");

// Force linking
Pass *polly::createSCoPConditionPass() {
  return new SCoPCondition();
}
