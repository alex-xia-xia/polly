//===------ CodeGeneration.cpp - Create LLVM IR from the CLooG AST --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Create LLVM IR code using the ClastParser.
//
//===----------------------------------------------------------------------===//
//

#define DEBUG_TYPE "polly-codegen"

#include "polly/ClastParser.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/SCoPHelper.h"
#include "polly/Support/BasicBlockEdge.h"
#include "polly/CLooG.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"


#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"

#include <vector>

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

using namespace polly;
using namespace llvm;

namespace polly {

typedef DenseMap<const Value*, Value*> ValueMapT;
typedef DenseMap<const char*, Value*> CharMapT;

// Create a new loop.
//
// @param Builder The builder used to create the loop.  It also defines the
//                place where to create the loop.
// @param UB      The upper bound of the loop iv.
// @param Stride  The number by which the loop iv is incremented after every
//                iteration.
static void createLoop(IRBuilder<> *Builder, Value *LB, Value *UB, APInt Stride,
                PHINode*& IV, BasicBlock*& AfterBB, Value*& IncrementedIV,
                DominatorTree *DT) {
  Function *F = Builder->GetInsertBlock()->getParent();
  LLVMContext &Context = F->getContext();

  BasicBlock *PreheaderBB = Builder->GetInsertBlock();
  BasicBlock *HeaderBB = BasicBlock::Create(Context, "polly.loop_header", F);
  BasicBlock *BodyBB = BasicBlock::Create(Context, "polly.loop_body", F);
  AfterBB = BasicBlock::Create(Context, "polly.after_loop", F);

  Builder->CreateBr(HeaderBB);
  DT->addNewBlock(HeaderBB, PreheaderBB);

  Builder->SetInsertPoint(BodyBB);

  Builder->SetInsertPoint(HeaderBB);

  // Use the type of UB, because IV will compare to UB.
  const IntegerType *LoopIVType = dyn_cast<IntegerType>(UB->getType());
  assert(LoopIVType && "UB is not integer?");

  // IV
  IV = Builder->CreatePHI(LoopIVType, "polly.loopiv");
  IV->addIncoming(LB, PreheaderBB);

  // IV increment.
  Stride.zext(64);
  Value *StrideValue = ConstantInt::get(Context, Stride);
  IncrementedIV = Builder->CreateAdd(IV, StrideValue, "polly.next_loopiv");

  // Exit condition.
  Value *CMP = Builder->CreateICmpSLE(IV, UB);
  Builder->CreateCondBr(CMP, BodyBB, AfterBB);
  DT->addNewBlock(BodyBB, HeaderBB);
  DT->addNewBlock(AfterBB, HeaderBB);

  Builder->SetInsertPoint(BodyBB);
}

// Insert a copy of a basic block in the newly generated code.
//
// @param Builder The builder used to insert the code. It also specifies
//                where to insert the code.
// @param BB      The basic block to copy
// @param VMap    A map returning for any old value its new equivalent. This
//                is used to update the operands of the statements.
//                For new statements a relation old->new is inserted in this
//                map.
//
static void copyBB (IRBuilder<> *Builder, BasicBlock *BB, ValueMapT &VMap,
                    DominatorTree *DT, const Region *R, ScalarEvolution *SE) {
  Function *F = Builder->GetInsertBlock()->getParent();
  LLVMContext &Context = F->getContext();

  BasicBlock *CopyBB = BasicBlock::Create(Context,
                                          "polly.stmt_" + BB->getNameStr(), F);
  Builder->CreateBr(CopyBB);
  DT->addNewBlock(CopyBB, Builder->GetInsertBlock());
  Builder->SetInsertPoint(CopyBB);

  std::vector<const Instruction*> ToCopy;

  ValueMapT BBMap;

  for (BasicBlock::const_iterator II = BB->begin(), IE = BB->end();
       II != IE; ++II)
    if (!II->isTerminator()) {
    const Instruction *Inst = &*II;
    bool Add = true;

    Instruction *NewInst = Inst->clone();

    // Replace old operands with the new ones.
    for (Instruction::op_iterator UI = NewInst->op_begin(),
         UE = NewInst->op_end(); UI != UE; ++UI)
      if (Instruction *OpInst = dyn_cast<Instruction>((*UI).get())) {
        // IVS and Parameters.
        if (VMap.find(OpInst) != VMap.end()) {
          Value *NewOp = VMap[*UI];

          // Insert a cast if the types differ.
          if ((*UI)->getType()->getScalarSizeInBits()
              < VMap[*UI]->getType()->getScalarSizeInBits())
            NewOp = Builder->CreateTruncOrBitCast(NewOp, (*UI)->getType());

          NewInst->replaceUsesOfWith(*UI, NewOp);

        // Instructions calculated in the current BB.
        } else if (BBMap.find(OpInst) != BBMap.end()) {
          Value *NewOp = BBMap[*UI];
          NewInst->replaceUsesOfWith(*UI, NewOp);

        // Ignore instructions that are referencing ops in the old BB. These
        // instructions are unused. They where replace by new ones during
        // createIndependentBlocks().
        } else if (R->contains(OpInst->getParent()))
          Add = false;
      }

    if (Add) {
      Builder->Insert(NewInst);
      BBMap[Inst] = NewInst;

      if (!NewInst->getType()->isVoidTy())
        NewInst->setName("p_" + Inst->getName());
    } else
      delete NewInst;

  }
}

/// Class to generate LLVM-IR that calculates the value of a clast_expr.
class ClastExpCodeGen {
  IRBuilder<> *Builder;
  const CharMapT *IVS;

  Value *codegen(clast_name *e, const Type *Ty) {
    CharMapT::const_iterator I = IVS->find(e->name);

    if (I != IVS->end())
      return Builder->CreateSExtOrBitCast(I->second, Ty);
    else
      llvm_unreachable("Clast name not found");
  }

  Value *codegen(clast_term *e, const Type *Ty) {
    APInt a = APInt_from_MPZ(e->val);

    Value *ConstOne = ConstantInt::get(Builder->getContext(), a);
    ConstOne = Builder->CreateSExtOrBitCast(ConstOne, Ty);

    if (e->var) {
      Value *var = codegen(e->var, Ty);
      return Builder->CreateMul(ConstOne, var);
    }

    return ConstOne;
  }

  Value *codegen(clast_binary *e, const Type *Ty) {
    Value *LHS = codegen(e->LHS, Ty);

    APInt RHS_AP = APInt_from_MPZ(e->RHS);

    Value *RHS = ConstantInt::get(Builder->getContext(), RHS_AP);
    RHS = Builder->CreateSExtOrBitCast(RHS, Ty);

    switch (e->type) {
    case clast_bin_mod:
      return Builder->CreateSRem(LHS, RHS);
    case clast_bin_fdiv:
      {
        // floord(n,d) ((n < 0) ? (n - d + 1) : n) / d
        Value *One = ConstantInt::get(Builder->getInt1Ty(), 1);
        Value *Zero = ConstantInt::get(Builder->getInt1Ty(), 0);
        One = Builder->CreateZExtOrBitCast(One, Ty);
        Zero = Builder->CreateZExtOrBitCast(Zero, Ty);
        Value *Sum1 = Builder->CreateSub(LHS, RHS);
        Value *Sum2 = Builder->CreateAdd(Sum1, One);
        Value *isNegative = Builder->CreateICmpSLT(LHS, Zero);
        Value *Dividend = Builder->CreateSelect(isNegative, Sum2, LHS);
        return Builder->CreateSDiv(Dividend, RHS);
      }
    case clast_bin_cdiv:
      {
        // ceild(n,d) ((n < 0) ? n : (n + d - 1)) / d
        Value *One = ConstantInt::get(Builder->getInt1Ty(), 1);
        Value *Zero = ConstantInt::get(Builder->getInt1Ty(), 0);
        One = Builder->CreateZExtOrBitCast(One, Ty);
        Zero = Builder->CreateZExtOrBitCast(Zero, Ty);
        Value *Sum1 = Builder->CreateAdd(LHS, RHS);
        Value *Sum2 = Builder->CreateSub(Sum1, One);
        Value *isNegative = Builder->CreateICmpSLT(LHS, Zero);
        Value *Dividend = Builder->CreateSelect(isNegative, LHS, Sum2);
        return Builder->CreateSDiv(Dividend, RHS);
      }
    case clast_bin_div:
      return Builder->CreateSDiv(LHS, RHS);
    default:
      llvm_unreachable("Unknown clast binary expression type");
    };
  }

  Value *codegen(clast_reduction *r, const Type *Ty) {
    assert((   r->type == clast_red_min
            || r->type == clast_red_max
            || r->type == clast_red_sum)
           && "Clast reduction type not supported");
    Value *old = codegen(r->elts[0], Ty);

    for (int i=1; i < r->n; ++i) {
      Value *exprValue = codegen(r->elts[i], Ty);

      switch (r->type) {
      case clast_red_min:
        {
          Value *cmp = Builder->CreateICmpSLT(old, exprValue);
          old = Builder->CreateSelect(cmp, old, exprValue);
          break;
        }
      case clast_red_max:
        {
          Value *cmp = Builder->CreateICmpSGT(old, exprValue);
          old = Builder->CreateSelect(cmp, old, exprValue);
          break;
        }
      case clast_red_sum:
        old = Builder->CreateAdd(old, exprValue);
        break;
      default:
        llvm_unreachable("Clast unknown reduction type");
      }
    }

    return old;
  }

public:

  // A generator for clast expressions.
  //
  // @param B The IRBuilder that defines where the code to calculate the
  //          clast expressions should be inserted.
  // @param IVMAP A Map that translates strings describing the induction
  //              variables to the Values* that represent these variables
  //              on the LLVM side.
  ClastExpCodeGen(IRBuilder<> *B, CharMapT *IVMap) : Builder(B), IVS(IVMap) {}

  // Generates code to calculate a given clast expression.
  //
  // @param e The expression to calculate.
  // @return The Value that holds the result.
  Value *codegen(clast_expr *e, const Type *Ty) {
    switch(e->type) {
      case clast_expr_name:
	return codegen((struct clast_name *)e, Ty);
      case clast_expr_term:
	return codegen((struct clast_term *)e, Ty);
      case clast_expr_bin:
	return codegen((struct clast_binary *)e, Ty);
      case clast_expr_red:
	return codegen((struct clast_reduction *)e, Ty);
      default:
        llvm_unreachable("Unknown clast expression!");
    }
  }

  Value *codegen(clast_expr *e) {
    return codegen(e, Builder->getInt64Ty());
  }
};

class ClastStmtCodeGen {
  // The SCoP we code generate.
  SCoP *S;

  DominatorTree *DT;
  ScalarEvolution *SE;

  // The Builder specifies the current location to code generate at.
  IRBuilder<> *Builder;

  // The current statement we are working on.
  //
  // Also used to track the assignment of old IVS to new values or
  // expressions.
  SCoPStmt *stmt;

  // Count the current assignment.  This is for user statements
  // to track how an IV from the old code corresponds to a value
  // or expression in the new code.
  //
  // There is one assignment with an empty LHS for every IV dimension
  // of each statement.
  unsigned assignmentCount;

  // Map the Values from the old code to their counterparts in the new code.
  ValueMapT ValueMap;

  // Codegenerator for clast expressions.
  ClastExpCodeGen ExpGen;

public:
  // Map the textual representation of variables in clast to the actual
  // Values* created during code generation.  It is used to map every
  // appearance of such a string in the clast to the value created
  // because of a loop iv or an assignment.
  CharMapT CharMap;

  protected:
  void codegen(struct clast_assignment *a) {
    Value *RHS = ExpGen.codegen(a->RHS);

    if (!a->LHS) {
      PHINode *PN = stmt->IVS[assignmentCount];
      assignmentCount++;
      Value *V = PN;

      if (PN->getNumOperands() == 2)
        V = *(PN->use_begin());

      ValueMap[V] = RHS;
    }
  }

  void codegen(struct clast_user_stmt *u) {
    SCoPStmt *UsrStmt= (SCoPStmt *)u->statement->usr;
    stmt = UsrStmt;
    BasicBlock *BB = stmt->getBasicBlock();
    assignmentCount = 0;

    // Actually we have a list of pointers here. Be careful.
    if (u->substitutions)
      codegen(u->substitutions);
    copyBB(Builder, BB, ValueMap, DT, &S->getRegion(), SE);
  }

  void codegen(struct clast_block *b) {
    if (b->body)
      codegen(b->body);
  }

  void codegen(struct clast_for *f) {
    APInt stride = APInt_from_MPZ(f->stride);
    PHINode *IV;
    Value *IncrementedIV;
    BasicBlock *AfterBB;
    Value *LB = ExpGen.codegen(f->LB);
    Value *UB = ExpGen.codegen(f->UB);
    createLoop(Builder, LB, UB, stride, IV, AfterBB, IncrementedIV,
               DT);
    (CharMap)[f->iterator] = IV;

    if (f->body)
      codegen(f->body);

    BasicBlock *HeaderBB = *pred_begin(AfterBB);
    BasicBlock *LastBodyBB = Builder->GetInsertBlock();
    Builder->CreateBr(HeaderBB);
    IV->addIncoming(IncrementedIV, LastBodyBB);
    Builder->SetInsertPoint(AfterBB);
  }

  Value *codegen(struct clast_equation *eq) {
    Value *LHS = ExpGen.codegen(eq->LHS);
    Value *RHS = ExpGen.codegen(eq->RHS);
    CmpInst::Predicate P;

    if (eq->sign == 0)
      P = ICmpInst::ICMP_EQ;
    else if (eq->sign > 0)
      P = ICmpInst::ICMP_SGE;
    else
      P = ICmpInst::ICMP_SLE;

    return Builder->CreateICmp(P, LHS, RHS);
  }

  void codegen(struct clast_guard *g) {
    Function *F = Builder->GetInsertBlock()->getParent();
    LLVMContext &Context = F->getContext();
    BasicBlock *ThenBB = BasicBlock::Create(Context, "polly.then", F);
    BasicBlock *MergeBB = BasicBlock::Create(Context, "polly.merge", F);
    DT->addNewBlock(ThenBB, Builder->GetInsertBlock());
    DT->addNewBlock(MergeBB, Builder->GetInsertBlock());

    Value *Predicate = codegen(&(g->eq[0]));

    for (int i = 1; i < g->n; ++i) {
      Value *TmpPredicate = codegen(&(g->eq[i]));
      Predicate = Builder->CreateAnd(Predicate, TmpPredicate);
    }

    Builder->CreateCondBr(Predicate, ThenBB, MergeBB);
    Builder->SetInsertPoint(ThenBB);

    codegen(g->then);

    Builder->CreateBr(MergeBB);
    Builder->SetInsertPoint(MergeBB);
  }

  void codegen(struct clast_root *r) { }

public:
  void codegen(clast_stmt *stmt) {
    if	    (CLAST_STMT_IS_A(stmt, stmt_root))
      codegen((struct clast_root *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_ass))
      codegen((struct clast_assignment *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_user))
      codegen((struct clast_user_stmt *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_block))
      codegen((struct clast_block *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_for))
      codegen((struct clast_for *)stmt);
    else if (CLAST_STMT_IS_A(stmt, stmt_guard))
      codegen((struct clast_guard *)stmt);

    if (stmt->next)
      codegen(stmt->next);
  }

  public:
  ClastStmtCodeGen(SCoP *scop, DominatorTree *dt, ScalarEvolution *se,
                   IRBuilder<> *B) : S(scop), DT(dt), SE(se), Builder(B),
  ExpGen(Builder, &CharMap) {}

};
}

namespace {
class ClastCodeGeneration : public RegionPass {
  Region *region;
  SCoP *S;
  DominatorTree *DT;
  ScalarEvolution *SE;
  CLooG *C;
  LoopInfo *LI;

  public:
  static char ID;

  ClastCodeGeneration() : RegionPass(&ID) {
    C = 0;
  }

  void createSeSeEdges(Region *R) {
    BasicBlock *newEntry = createSingleEntryEdge(R, this);

    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      if ((*SI)->getBasicBlock() == R->getEntry())
        (*SI)->setBasicBlock(newEntry);

    createSingleExitEdge(R, this);
  }

  void addParameters(CloogNames *names, CharMapT &VariableMap,
                     IRBuilder<> *Builder) {
    int i = 0;
    SCEVExpander Rewriter(*SE);

    for (SCoP::param_iterator PI = S->param_begin(), PE = S->param_end();
         PI != PE; ++PI) {
      assert(i < names->nb_parameters && "Not enough parameter names");

      const SCEV *Param = *PI;
      const Type *Ty = Param->getType();
      Instruction *InsertInst = &(*Builder->GetInsertBlock()->begin());

      VariableMap[names->parameters[i]] = Rewriter.expandCodeFor(Param, Ty,
                                                                 InsertInst);
      ++i;
    }
  }

  // Create new code for every instruction operator that can be expressed by a
  // SCEV.  Like this there are just two types of instructions left:
  //
  // 1. Instructions that only reference loop ivs or parameters outside the
  // region.
  //
  // 2. Instructions that are not used for any memory modification. (These
  //    will be ignored later on.)
  //
  // Blocks containing only these kind of instructions are called independent
  // blocks as they can be scheduled arbitrarily.
  void createIndependentBlocks(BasicBlock *BB) {
    std::vector<Instruction*> work;
    SCEVExpander Rewriter(*SE);
    Rewriter.disableInstructionHoisting();

    for (BasicBlock::iterator II = BB->begin(), IE = BB->end();
         II != IE; ++II) {
        Instruction *Inst = &*II;

        if (!SE->isSCEVable(Inst->getType()) || isa<StoreInst>(Inst)
            || isa<LoadInst>(Inst))
          work.push_back(Inst);
      }

    for (std::vector<Instruction*>::iterator II = work.begin(), IE = work.end();
         II != IE; ++II) {
      Instruction *Inst = *II;

      for (Instruction::op_iterator UI = Inst->op_begin(),
           UE = Inst->op_end(); UI != UE; ++UI) {
        if (!SE->isSCEVable(UI->get()->getType()))
          continue;

        const SCEV *Scev = SE->getSCEV(*UI);

        Value *V = Rewriter.expandCodeFor(Scev, UI->get()->getType(), Inst);
        UI->set(V);
      }
    }
  }

  void createIndependentBlocks(SCoP *S) {
    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      createIndependentBlocks((*SI)->getBasicBlock());
  }

  bool isIV(Instruction *I) {
    Loop *L = LI->getLoopFor(I->getParent());

    return L && I == L->getCanonicalInductionVariable();
  }

  bool isIndependentBlock(const Region *R, BasicBlock *BB) {
    for (BasicBlock::iterator II = BB->begin(), IE = BB->end();
         II != IE; ++II) {
      Instruction *Inst = &*II;

      if (isIV(Inst))
        continue;

      // A value inside the SCoP is referenced outside.
      for (Instruction::use_iterator UI = Inst->use_begin(),
           UE = Inst->use_end(); UI != UE; ++UI) {
        Value *V = *UI;
        Instruction *Use = dyn_cast<Instruction>(V);

        if (Use && !R->contains(Use)) {
          DEBUG(dbgs() << "Instruction not independent:\n");
          DEBUG(dbgs() << "Instruction used outside the SCoP!\n");
          DEBUG(Inst->print(dbgs()));
          DEBUG(dbgs() << "\n");
          return false;
        }
      }

      for (Instruction::op_iterator UI = Inst->op_begin(),
           UE = Inst->op_end(); UI != UE; ++UI) {
        Instruction *Op = dyn_cast<Instruction>(&(*UI));

        if (Op && R->contains(Op) && !(Op->getParent() == BB)
            && !isIV(Op)) {
          DEBUG(dbgs() << "Instruction not independent:\n");
          DEBUG(dbgs() << "Uses invalid operator\n");
          DEBUG(Inst->print(dbgs()));
          DEBUG(dbgs() << "\n");
          DEBUG(dbgs() << "Invalid operator is: " << Op->getNameStr() << "\n");
          return false;
        }
      }
    }

    return true;
  }

  bool hasIndependentBlocks(SCoP *S) {
    for (SCoP::iterator SI = S->begin(), SE = S->end(); SI != SE; ++SI)
      if (!isIndependentBlock(&S->getRegion(), (*SI)->getBasicBlock()))
        return false;

    return true;
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();
    SE = &getAnalysis<ScalarEvolution>();
    LI = &getAnalysis<LoopInfo>();

    if (!S) {
      C = 0;
      return false;
    }

    // Parameters not yet supported.
    if (S->param_begin() != S->param_end()) {
      errs() << "Code generation for SCoP " << S->getRegion().getNameStr()
        << " failed. Parameters not yet supported.\n";
      return false;
    }

    createIndependentBlocks(S);

    if (!hasIndependentBlocks(S)) {
      errs() << "Code generation for SCoP " << S->getRegion().getNameStr()
        << " failed. Could not generate independent blocks.\n";
      return false;
    }

    createSeSeEdges(R);

    if (C)
      delete(C);

    C = new CLooG(S);
    C->generate();

    Function *F = R->getEntry()->getParent();

    // Create a basic block in which to start code generation.
    BasicBlock *PollyBB = BasicBlock::Create(F->getContext(), "pollyBB", F);
    IRBuilder<> Builder(PollyBB);
    DT->addNewBlock(PollyBB, R->getEntry());

    ClastStmtCodeGen CodeGen(S, DT, SE, &Builder);

    clast_stmt *clast = C->getClast();

    addParameters(((clast_root*)clast)->names, CodeGen.CharMap, &Builder);

    CodeGen.codegen(clast);

    BasicBlock *AfterSCoP = *pred_begin(R->getExit());
    Builder.CreateBr(AfterSCoP);

    // Update old PHI nodes to pass LLVM verification.
    for (BasicBlock::iterator SI = succ_begin(R->getEntry())->begin(),
         SE = succ_begin(R->getEntry())->end(); SI != SE; ++SI)
      if (PHINode::classof(&(*SI))) {
        PHINode *PN = static_cast<PHINode *>(&*SI);
        PN->removeIncomingValue(R->getEntry());
      }

    if (DT->dominates(R->getEntry(), R->getExit()))
      DT->changeImmediateDominator(AfterSCoP, Builder.GetInsertBlock());

    // Enable the new polly code.
    R->getEntry()->getTerminator()->setSuccessor(0, PollyBB);

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
    if (C)
      C->pprint();
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<SCoPInfo>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<ScalarEvolution>();
    AU.addRequired<LoopInfo>();
    AU.addPreserved<DominatorTree>();
    AU.addPreserved<SCoPInfo>();
    AU.addPreserved<RegionInfo>();
    AU.setPreservesAll();
  }
};
}

char ClastCodeGeneration::ID = 1;

static RegisterPass<ClastCodeGeneration>
Z("polly-codegen", "Polly - Create LLVM-IR from the polyhedral "
                         "information");

RegionPass* polly::createClastCodeGenerationPass() {
	return new ClastCodeGeneration();
}