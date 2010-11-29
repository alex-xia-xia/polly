//===------ CodeGeneration.cpp - Code generate the SCoPs. -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The CodeGeneration pass takes a SCoP created by SCoPInfo and translates it
// back to LLVM-IR using CLooG.
//
// The SCoP describes the high level memory behaviour of a control flow region.
// Transformation passes can update the schedule (execution order) of statements
// in the SCoP. CLooG is used to generate an abstract syntax tree (clast) that
// reflects the updated execution order. This clast is used to create new
// LLVM-IR that is computational equivalent to the original control flow region,
// but executes its code in the new execution order defined by the changed
// scattering.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "polly-codegen"

#include "polly/LinkAllPasses.h"
#include "polly/Support/GmpConv.h"
#include "polly/Support/SCoPHelper.h"
#include "polly/CLooG.h"
#include "polly/Dependences.h"
#include "polly/SCoPInfo.h"
#include "polly/TempSCoPInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Module.h"

#define CLOOG_INT_GMP 1
#include "cloog/cloog.h"
#include "cloog/isl/cloog.h"

#include <vector>
#include <utility>

using namespace polly;
using namespace llvm;

namespace polly {

static cl::opt<bool>
OpenMP("polly-codegen-parallel",
       cl::desc("Generate OpenMP parallel code"), cl::Hidden,
       cl::value_desc("OpenMP code generation enabled if true"),
       cl::ValueRequired, cl::init(false));

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

  // Use the type of upper and lower bound.
  assert(LB->getType() == UB->getType()
         && "Different types for upper and lower bound.");

  const IntegerType *LoopIVType = dyn_cast<IntegerType>(UB->getType());
  assert(LoopIVType && "UB is not integer?");

  // IV
  IV = Builder->CreatePHI(LoopIVType, "polly.loopiv");
  IV->addIncoming(LB, PreheaderBB);

  // IV increment.
  Stride.zext(LoopIVType->getBitWidth());
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
static void copyBB(IRBuilder<> *Builder, BasicBlock *BB, ValueMapT &VMap,
                   DominatorTree *DT, const Region *R) {
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

    // Copy the operands in temporary vector, as an in place update
    // fails if an instruction is referencing the same operand twice.
    std::vector<Value*> Operands(NewInst->op_begin(), NewInst->op_end());

    // Replace old operands with the new ones.
    for (std::vector<Value*>::iterator UI = Operands.begin(),
         UE = Operands.end(); UI != UE; ++UI)
      if (Instruction *OpInst = dyn_cast<Instruction>((*UI))) {
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
        } else if (R->contains(OpInst->getParent())) {
          assert(!isa<StoreInst>(NewInst)
                 && "Store instructions are always needed!");
          Add = false;
        }
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
  Dependences *DP;

  // The Builder specifies the current location to code generate at.
  IRBuilder<> *Builder;

  // Map the Values from the old code to their counterparts in the new code.
  ValueMapT ValueMap;

  // Codegenerator for clast expressions.
  ClastExpCodeGen ExpGen;

  // Do we currently generate parallel code?
  bool parallelCodeGeneration;

public:
  // Map the textual representation of variables in clast to the actual
  // Values* created during code generation.  It is used to map every
  // appearance of such a string in the clast to the value created
  // because of a loop iv or an assignment.
  CharMapT CharMap;

  protected:
  void codegen(struct clast_assignment *a, SCoPStmt *Statement = 0,
               unsigned Dimension = 0) {
    Value *RHS = ExpGen.codegen(a->RHS);

    if (!a->LHS) {
      assert(Statement && "Empty statement in assignment of a substitution.");
      const PHINode *PN;
      PN = Statement->getInductionVariableForDimension(Dimension);
      const Value *V = PN;

      if (PN->getNumOperands() == 2)
        V = *(PN->use_begin());

      ValueMap[V] = RHS;
    }
  }

  void codegenSubstitutions(struct clast_stmt *Assignment,
                            SCoPStmt *Statement) {
    int Dimension = 0;

    while (Assignment) {
      assert(CLAST_STMT_IS_A(Assignment, stmt_ass)
             && "Substitions are expected to be assignments");
      codegen((struct clast_assignment *)Assignment, Statement, Dimension);
      Assignment = Assignment->next;
      Dimension++;
    }
  }

  void codegen(struct clast_user_stmt *u) {
    SCoPStmt *Statement = (SCoPStmt *)u->statement->usr;
    BasicBlock *BB = Statement->getBasicBlock();

    if (u->substitutions)
      codegenSubstitutions(u->substitutions, Statement);

    copyBB(Builder, BB, ValueMap, DT, &S->getRegion());
  }

  void codegen(struct clast_block *b) {
    if (b->body)
      codegen(b->body);
  }

  /// @brief Create a classical sequential loop.
  void codegenForSequential(struct clast_for *f) {
    APInt Stride = APInt_from_MPZ(f->stride);
    PHINode *IV;
    Value *IncrementedIV;
    BasicBlock *AfterBB;
    Value *LB = ExpGen.codegen(f->LB);
    Value *UB = ExpGen.codegen(f->UB);
    createLoop(Builder, LB, UB, Stride, IV, AfterBB, IncrementedIV, DT);
    (CharMap)[f->iterator] = IV;

    if (f->body)
      codegen(f->body);

    BasicBlock *HeaderBB = *pred_begin(AfterBB);
    BasicBlock *LastBodyBB = Builder->GetInsertBlock();
    Builder->CreateBr(HeaderBB);
    IV->addIncoming(IncrementedIV, LastBodyBB);
    Builder->SetInsertPoint(AfterBB);
  }

  /// @brief Check if a loop is parallel
  ///
  /// Detect if a clast_for loop can be executed in parallel.
  ///
  /// @param f The clast for loop to check.
  bool isParallelFor(struct clast_for *f) {
    isl_set *loopDomain = isl_set_from_cloog_domain(f->domain);

    // Get the scattering dimension this loop enumerates.
    std::string DimName(f->iterator);
    DimName.erase(0, 1);
    int scatteringLevel = atoi(DimName.c_str());

    // Check if it is parallel.
    bool isParallel = DP->isParallelDimension(loopDomain, scatteringLevel - 1);

    if (isParallel)
      DEBUG(errs() << "Parallel loop with iv c" << scatteringLevel
            << " found\n";);

    return isParallel;
  }

  /// @brief Create an OpenMP parallel for loop.
  ///
  /// This loop reflects a loop as if it would have been created by an OpenMP
  /// statement.
  void codegenForOpenMP(struct clast_for *f) {
    Module *M = Builder->GetInsertBlock()->getParent()->getParent();
    Function *FN = M->getFunction("GOMP_parallel_end");

    Builder->CreateCall(FN);
  }

  void codegen(struct clast_for *f) {
    if (OpenMP && !parallelCodeGeneration && isParallelFor(f)) {
      parallelCodeGeneration = true;
      codegenForOpenMP(f);
      parallelCodeGeneration = false;
    } else
      codegenForSequential(f);
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

  void codegen(struct clast_root *r) {
    parallelCodeGeneration = false;
  }

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
  ClastStmtCodeGen(SCoP *scop, DominatorTree *dt, Dependences *dp,
                   IRBuilder<> *B) :
    S(scop), DT(dt), DP(dp), Builder(B),
  ExpGen(Builder, &CharMap) {}

};
}

namespace {
class CodeGeneration : public RegionPass {
  Region *region;
  SCoP *S;
  DominatorTree *DT;
  ScalarEvolution *SE;
  SCoPDetection *SD;
  CLooG *C;
  LoopInfo *LI;

  public:
  static char ID;

  CodeGeneration() : RegionPass(ID) {
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

    // Create an instruction that specifies the location where the parameters
    // are expanded.
    CastInst::CreateIntegerCast(ConstantInt::getTrue(Builder->getContext()),
                                  Builder->getInt16Ty(), false, "insertInst",
                                  Builder->GetInsertBlock());

    for (SCoP::param_iterator PI = S->param_begin(), PE = S->param_end();
         PI != PE; ++PI) {
      assert(i < names->nb_parameters && "Not enough parameter names");
      Instruction *InsertLocation;

      const SCEV *Param = *PI;
      const Type *Ty = Param->getType();

      InsertLocation = --(Builder->GetInsertBlock()->end());
      Value *V = Rewriter.expandCodeFor(Param, Ty, InsertLocation);
      VariableMap[names->parameters[i]] = V;

      ++i;
    }
  }

  void addOpenMPDefinitions(IRBuilder<> *Builder)
  {
      // Adding prototypes required if OpenMP is enabled.
      Module *M = Builder->GetInsertBlock()->getParent()->getParent();
      Function *FN = M->getFunction("GOMP_parallel_end");

      // Check if the definition is already added. Otherwise add it.
      if (!FN) {
          LLVMContext &Context = Builder->getContext();
          FunctionType *FT = FunctionType::get(Type::getVoidTy(Context),
                  std::vector<const Type*>(), false);
          Function::Create(FT, Function::ExternalLinkage, 
                  "GOMP_parallel_end", M); 
      }
  }

  bool runOnRegion(Region *R, RGPassManager &RGM) {
    region = R;
    S = getAnalysis<SCoPInfo>().getSCoP();
    DT = &getAnalysis<DominatorTree>();
    Dependences *DP = &getAnalysis<Dependences>();
    SE = &getAnalysis<ScalarEvolution>();
    LI = &getAnalysis<LoopInfo>();
    SD = &getAnalysis<SCoPDetection>();

    if (!S) {
      C = 0;
      return false;
    }

    createSeSeEdges(R);

    if (C)
      delete(C);

    C = new CLooG(S);

    Function *F = R->getEntry()->getParent();

    // Create a basic block in which to start code generation.
    BasicBlock *PollyBB = BasicBlock::Create(F->getContext(), "pollyBB", F);
    IRBuilder<> Builder(PollyBB);
    DT->addNewBlock(PollyBB, R->getEntry());

    ClastStmtCodeGen CodeGen(S, DT, DP, &Builder);

    clast_stmt *clast = C->getClast();

    addParameters(((clast_root*)clast)->names, CodeGen.CharMap, &Builder);

    if (OpenMP)
      addOpenMPDefinitions(&Builder);

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

    DT->changeImmediateDominator(AfterSCoP, Builder.GetInsertBlock());

    BasicBlock *OldRegionEntry = *succ_begin(R->getEntry());

    // Enable the new polly code.
    R->getEntry()->getTerminator()->setSuccessor(0, PollyBB);

    // Remove old SCoP nodes from dominator tree.
    std::vector<DomTreeNode*> ToVisit;
    std::vector<DomTreeNode*> Visited;
    ToVisit.push_back(DT->getNode(OldRegionEntry));

    while (!ToVisit.empty()) {
      DomTreeNode *Node = ToVisit.back();

      ToVisit.pop_back();

      if (AfterSCoP == Node->getBlock())
        continue;

      Visited.push_back(Node);

      std::vector<DomTreeNode*> Children = Node->getChildren();
      ToVisit.insert(ToVisit.end(), Children.begin(), Children.end());
    }

    for (std::vector<DomTreeNode*>::reverse_iterator I = Visited.rbegin(),
         E = Visited.rend(); I != E; ++I)
      DT->eraseNode((*I)->getBlock());

    R->getParent()->removeSubRegion(R);

    // And forget the SCoP if we remove the region.
    SD->forgetSCoP(*R);

    return false;
  }

  void print(raw_ostream &OS, const Module *) const {
    if (C)
      C->pprint();
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<Dependences>();
    AU.addRequired<DominatorTree>();
    AU.addRequired<ScalarEvolution>();
    AU.addRequired<LoopInfo>();
    AU.addRequired<RegionInfo>();
    AU.addRequired<SCoPDetection>();
    AU.addRequired<SCoPInfo>();

    AU.addPreserved<Dependences>();
    AU.addPreserved<LoopInfo>();
    AU.addPreserved<DominatorTree>();
    AU.addPreserved<PostDominatorTree>();
    AU.addPreserved<SCoPDetection>();
    AU.addPreserved<ScalarEvolution>();
    AU.addPreserved<RegionInfo>();
    AU.addPreserved<SCoPInfo>();
    AU.addPreservedID(IndependentBlocksID);
  }
};
}

char CodeGeneration::ID = 1;

static RegisterPass<CodeGeneration>
Z("polly-codegen", "Polly - Create LLVM-IR from the polyhedral information");

Pass* polly::createCodeGenerationPass() {
  return new CodeGeneration();
}
