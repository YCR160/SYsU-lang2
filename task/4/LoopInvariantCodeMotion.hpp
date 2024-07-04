#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"

class LoopInvariantCodeMotion : public llvm::PassInfoMixin<LoopInvariantCodeMotion>
{
  public:
    explicit LoopInvariantCodeMotion(llvm::raw_ostream &out) : mOut(out)
    {
    }

    llvm::PreservedAnalyses run(llvm::Module &mod, llvm::ModuleAnalysisManager &mam);

  private:
    llvm::raw_ostream &mOut;
};
