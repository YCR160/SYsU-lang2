#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "map"

class FunctionInlining : public llvm::PassInfoMixin<FunctionInlining>
{
  public:
    explicit FunctionInlining(llvm::raw_ostream &out) : mOut(out)
    {
    }

    llvm::PreservedAnalyses run(llvm::Module &mod, llvm::ModuleAnalysisManager &mam);

  private:
    llvm::raw_ostream &mOut;
};
