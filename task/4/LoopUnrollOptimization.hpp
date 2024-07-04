#pragma once

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

class LoopUnrollOptimization : public llvm::PassInfoMixin<LoopUnrollOptimization>
{
  public:
    explicit LoopUnrollOptimization(llvm::raw_ostream &out) : mOut(out)
    {
    }

    llvm::PreservedAnalyses run(llvm::Module &mod, llvm::ModuleAnalysisManager &mam);

  private:
    llvm::raw_ostream &mOut;

    void unrollLoop(llvm::Loop *LP);
};
