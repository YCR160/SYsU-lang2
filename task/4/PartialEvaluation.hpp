#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "unordered_map"

class PartialEvaluation : public llvm::PassInfoMixin<PartialEvaluation>
{
  public:
    explicit PartialEvaluation(llvm::raw_ostream &out) : mOut(out)
    {
    }

    llvm::PreservedAnalyses run(llvm::Module &mod, llvm::ModuleAnalysisManager &mam);

  private:
    llvm::raw_ostream &mOut;
};
