//===--- ScopDetection.h - Detect Scops -------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Detect the maximal Scops of a function.
//
// A static control part (Scop) is a subgraph of the control flow graph (CFG)
// that only has statically known control flow and can therefore be described
// within the polyhedral model.
//
// Every Scop fullfills these restrictions:
//
// * It is a single entry single exit region
//
// * Only affine linear bounds in the loops
//
// Every natural loop in a Scop must have a number of loop iterations that can
// be described as an affine linear function in surrounding loop iterators or
// parameters. (A parameter is a scalar that does not change its value during
// execution of the Scop).
//
// * Only comparisons of affine linear expressions in conditions
//
// * All loops and conditions perfectly nested
//
// The control flow needs to be structured such that it could be written using
// just 'for' and 'if' statements, without the need for any 'goto', 'break' or
// 'continue'.
//
// * Side effect free functions call
//
// Only function calls and intrinsics that do not have side effects are allowed
// (readnone).
//
// The Scop detection finds the largest Scops by checking if the largest
// region is a Scop. If this is not the case, its canonical subregions are
// checked until a region is a Scop. It is now tried to extend this Scop by
// creating a larger non canonical region.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_SCOP_DETECTION_H
#define POLLY_SCOP_DETECTION_H

#include "llvm/Pass.h"

#include <set>

using namespace llvm;

namespace llvm {
  class RegionInfo;
  class Region;
  class LoopInfo;
  class Loop;
  class ScalarEvolution;
  class SCEV;
  class SCEVAddRecExpr;
  class CallInst;
  class Instruction;
}

namespace polly {
typedef std::set<const SCEV*> ParamSetType;

//===----------------------------------------------------------------------===//
/// @brief Pass to detect the maximal static control parts (Scops) of a
/// function.
class ScopDetection : public FunctionPass {
  //===--------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  ScopDetection(const ScopDetection &);
  // DO NOT IMPLEMENT
  const ScopDetection &operator=(const ScopDetection &);

  /// @brief Analysis passes used.
  //@{
  ScalarEvolution* SE;
  LoopInfo *LI;
  RegionInfo *RI;
  //@}

  // Remember the valid regions
  typedef std::set<const Region*> RegionSet;
  RegionSet ValidRegions;

  // Try to expand the region R. If R can be expanded return the expanded
  // region, NULL otherwise.
  Region *expandRegion(Region &R);

  // Find the Scops in this region tree.
  void findScops(Region &R);

  /// @brief Check if all basic block in the region are valid.
  ///
  /// @param R The region to check.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  /// @return True if all blocks in R are valid, false otherwise.
  bool allBlocksValid(Region &R, bool verifying) const;

  /// @brief Check the exit block of a region is valid.
  ///
  /// @param R The region to check.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  /// @return True if the exit of R is valid, false otherwise.
  bool isValidExit(Region &R, bool verifying) const;

  /// @brief Check if a region is a Scop.
  ///
  /// @param R The region to check.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  /// @return True if R is a Scop, false otherwise.
  bool isValidRegion(Region &R, bool verifying) const;

  /// @brief Check if a call instruction can be part of a Scop.
  ///
  /// @param CI The call instruction to check.
  /// @return True if the call instruction is valid, false otherwise.
  static bool isValidCallInst(CallInst &CI);

  /// @brief Check if a memory access can be part of a Scop.
  ///
  /// @param Inst The instruction accessing the memory.
  /// @param RefRegion The region in respect to which we check the access
  ///                  function.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  /// @return True if the memory access is valid, false otherwise.
  bool isValidMemoryAccess(Instruction &Inst, Region &RefRegion,
                           bool verifying) const;

  /// @brief Check if an instruction has any non trivial scalar dependencies
  ///        as part of a Scop.
  ///
  /// @param Inst The instruction to check.
  /// @param RefRegion The region in respect to which we check the access
  ///                  function.
  /// @return True if the instruction has scalar dependences, false otherwise.
  bool hasScalarDependency(Instruction &Inst, Region &RefRegion) const;

  /// @brief Check if an instruction can be part of a Scop.
  ///
  /// @param Inst The instruction to check.
  /// @param RefRegion The region in respect to which we check the instruction.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  /// @return True if the instruction is valid, false otherwise.
  bool isValidInstruction(Instruction &I, Region &RefRegion,
                          bool verifying) const;

  /// @brief Check if the BB can be part of a Scop.
  ///
  /// @param BB The basic block to check.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  /// @return True if the basic block is valid, false otherwise.
  bool isValidBasicBlock(BasicBlock &BB, Region &RefRegion,
                         bool verifying) const;

  /// @brief Check if the control flow in a basic block is valid.
  ///
  /// @param BB The BB to check the control flow.
  /// @param RefRegion The region in respect to which we check the control
  ///                  flow.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  ///
  /// @return True if the BB contains only valid control flow.
  bool isValidCFG(BasicBlock &BB, Region &RefRegion,
                  bool verifying) const;

  /// @brief Check if the SCEV expression is a valid affine function
  ///
  /// @param S          The SCEV expression to be checked
  /// @param RefRegion  The reference scope to check SCEV, it help to find out
  ///                   induction variables and parameters
  /// @param isMemoryAccess Does S represent a memory access. In this case one
  ///                       base pointer is allowed.
  ///
  /// @return True if the SCEV expression is affine, false otherwise
  ///
  bool isValidAffineFunction(const SCEV *S, Region &RefRegion,
                             bool isMemoryAccess = false) const;

  /// @brief Is a loop valid with respect to a given region.
  ///
  /// @param L The loop to check.
  /// @param RefRegion The region we analyse the loop in.
  /// @param verifying Should the Scop be verified? In this case we error,
  ///                  if this is no Scop.
  ///
  /// @return True if the loop is valid in the region.
  bool isValidLoop(Loop *L, Region &RefRegion,
                   bool verifying) const;

public:
  static char ID;
  explicit ScopDetection() : FunctionPass(ID) {}

  /// @brief Get the RegionInfo stored in this pass.
  ///
  /// This was added to give the DOT printer easy access to this information.
  RegionInfo *getRI() const { return RI; }

  /// @brief Is the region is the maximum region of a Scop?
  ///
  /// @param R The Region to test if it is maximum.
  ///
  /// @return Return true if R is the maximum Region in a Scop, false otherwise.
  bool isMaxRegionInScop(const Region &R) const;

  /// @name Maximum Region In Scops Iterators
  ///
  /// These iterators iterator over all maximum region in Scops of this
  /// function.
  //@{
  typedef RegionSet::iterator iterator;
  typedef RegionSet::const_iterator const_iterator;

  iterator begin()  { return ValidRegions.begin(); }
  iterator end()    { return ValidRegions.end();   }

  const_iterator begin() const { return ValidRegions.begin(); }
  const_iterator end()   const { return ValidRegions.end();   }
  //@}

  /// @brief Remove a region and its children from valid region set.
  ///
  /// @param R The region to remove.
  void forgetScop(const Region &R) {
    assert(isMaxRegionInScop(R) && "R is not a Scop!");
    ValidRegions.erase(&R);
  }

  /// @brief Verify if all valid Regions in this Function are still valid
  /// after some transformations.
  void verifyAnalysis() const;

  /// @brief Verify if R is still a valid part of Scop after some
  /// transformations.
  ///
  /// @param R The Region to verify.
  void verifyRegion(const Region &R) const;

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory();
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

} //end namespace polly

#endif