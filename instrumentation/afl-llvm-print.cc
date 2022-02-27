// Copyright Siddharth (2021)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <string>
#include <fstream>
#include <sys/time.h>
#include <set>

#include "llvm/Config/llvm-config.h"
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 5
typedef long double max_align_t;
#endif

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#if LLVM_VERSION_MAJOR >= 4 || \
    (LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 4)
  #include "llvm/IR/DebugInfo.h"
  #include "llvm/IR/CFG.h"
#else
  #include "llvm/DebugInfo.h"
  #include "llvm/Support/CFG.h"
#endif

using namespace llvm;

namespace {

class AFLPrint : public ModulePass {
  private:
    void ParseFunction(Function &F, std::set<Function *> &visited);
    bool isInstrumentedBB(BasicBlock &BB);
    int getBBId(BasicBlock &BB);
  public:
    static char ID;
    AFLPrint() : ModulePass(ID) {

    }
    bool runOnModule(Module &M) override;
};

} // namespace
char AFLPrint::ID = 0;
bool AFLPrint::runOnModule(Module &M) {
    LLVMContext &C = M.getContext();
    std::set<llvm::Function *> visited;
    for (auto &TF : M) {
      // since we don't care about non declared functions
      if (TF.isDeclaration()) continue;
      if (visited.find(&TF) != visited.end()) continue;

      // Newly visited functions
      visited.insert(&TF);
      ParseFunction(TF, visited);
    }

    llvm::dbgs() << "AFLPrint: " << M.getName() << "\n";
    return false;
}

void AFLPrint::ParseFunction(Function &F, std::set<Function *> &visited) {
  // Iterate over basic blocks
  // Here we need to do a BFS and go recursively when we encounter a function call
  std::set<BasicBlock *> visitedBB;

  BasicBlock *currBB = &F.getEntryBlock();

  // Let's assume that the block instrumentation is at the top of the block, including the value that's stored



  for (auto &BB : F) {
    if (isInstrumentedBB(BB)) {
      int id = getBBId(BB);
      llvm::dbgs() << "ID is " << id << "\n";
    }
  }
}

bool AFLPrint::isInstrumentedBB(BasicBlock &BB) {
  // Iterate over instructions
  for (auto &I : BB) {
    if (LoadInst *LI = dyn_cast<LoadInst> (&I)) {
      Value *PO = LI->getPointerOperand();
      if (PO->hasName() && PO->getName().startswith("__afl")) {
        return true;
      }
    }
  }
  return false;
}

int AFLPrint::getBBId(BasicBlock &BB) {
  for (auto &I : BB) {
    // if (StoreInst *SI = dyn_cast<StoreInst> (&I)) {
    //   // Fuck! we lose the last bit when we right shift by 1, need to back track the xor
    //   // if (SI->getPointerOperand()->getName() == "__afl_prev_loc") {
    //   //   ConstantInt *CI = dyn_cast<ConstantInt>(SI->getValueOperand());
    //   //   return CI->getSExtValue();
    //   // }
    // }
    // We get the value where __afl_prev_loc is stored
    // follow all uses to find a XOR instruction
    // Get the 2nd operand of the XOR instruction
    if (LoadInst *LI = dyn_cast<LoadInst> (&I)) {
      Value *PO = LI->getPointerOperand();
      if (PO->hasName() && PO->getName() == "__afl_prev_loc") {
        Instruction *NI = LI->getNextNode()->getNextNode();
        if (BinaryOperator *BO = dyn_cast<BinaryOperator>(NI)) {
          if (BO->getOpcode() == Instruction::Xor) {
            BO->dump();
            ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1));
            return CI->getSExtValue();
          }
        }

        // Value *V = dyn_cast<Value>(&I);
        // for (Value::use_iterator NI = V->use_begin(), EI = V->use_end(); NI != EI; ++NI) {
        //   (*NI)->dump();
        //   if (BinaryOperator *BO = dyn_cast<BinaryOperator>(*NI)) {
        //     if (BO->getOpcode() == Instruction::Xor) {
        //       BO->dump();
        //       ConstantInt *CI = dyn_cast<ConstantInt>(BO->getOperand(1));
        //       return CI->getSExtValue();
        //     }
        //   }
        // }
      }
    }
  }
  return -1;
}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLPrint());

}

static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_OptimizerLast, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
