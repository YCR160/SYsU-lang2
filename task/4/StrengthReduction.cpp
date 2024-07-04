#include "StrengthReduction.hpp"

using namespace llvm;

PreservedAnalyses StrengthReduction::run(Module &mod, ModuleAnalysisManager &mam)
{
    int strengthReductionTimes = 0;

    for (auto &func : mod)
    {
        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                if (auto *binOp = dyn_cast<BinaryOperator>(&inst))
                {
                    auto *lhs = binOp->getOperand(0);
                    auto *rhs = binOp->getOperand(1);
                    switch (binOp->getOpcode())
                    {
                    case Instruction::UDiv: {
                        // a = a / a -> a = 1
                        if (lhs == rhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::get(binOp->getType(), 1));
                            ++strengthReductionTimes;
                        }
                        break;
                    }
                    case Instruction::SDiv: {
                        if (lhs == rhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::get(binOp->getType(), 1));
                            ++strengthReductionTimes;
                        }
                        break;
                    }
                    // a = a * x -> a = a << log2(x)
                    case Instruction::Mul: {
                        if (auto *constRhs = dyn_cast<ConstantInt>(rhs))
                        {
                            APInt multiplierValue = constRhs->getValue();
                            if (multiplierValue.isPowerOf2())
                            {
                                int shift = multiplierValue.logBase2();
                                Value *newInst = BinaryOperator::Create(
                                    Instruction::Shl, lhs, ConstantInt::get(constRhs->getType(), shift), "", binOp);
                                binOp->replaceAllUsesWith(newInst);
                                ++strengthReductionTimes;
                            }
                        }
                        break;
                    }
                    // a = a % x -> a = a & (x - 1)
                    case Instruction::URem: {
                        if (auto *constRhs = dyn_cast<ConstantInt>(rhs))
                        {
                            APInt divisorValue = constRhs->getValue();
                            if (divisorValue == 1)
                            {
                                binOp->replaceAllUsesWith(ConstantInt::get(constRhs->getType(), 0));
                                ++strengthReductionTimes;
                            }
                            else if (divisorValue.isPowerOf2())
                            {
                                binOp->replaceAllUsesWith(BinaryOperator::Create(
                                    Instruction::And, lhs, ConstantInt::get(constRhs->getType(), divisorValue - 1), "",
                                    binOp));
                                ++strengthReductionTimes;
                            }
                        }
                        break;
                    }
                    case Instruction::SRem: {
                        if (auto *constRhs = dyn_cast<ConstantInt>(rhs))
                        {
                            APInt divisorValue = constRhs->getValue();
                            if (divisorValue == 1)
                            {
                                binOp->replaceAllUsesWith(ConstantInt::get(constRhs->getType(), 0));
                                ++strengthReductionTimes;
                            }
                        }
                        break;
                    }
                    // a = b / x + c / x -> a = (b + c) / x
                    case Instruction::Add: {
                        if (lhs == rhs)
                        {
                            Value *newInst = BinaryOperator::Create(Instruction::Mul, lhs,
                                                                    ConstantInt::get(binOp->getType(), 2), "", binOp);
                            binOp->replaceAllUsesWith(newInst);
                            ++strengthReductionTimes;
                        }
                        else if (auto *addOp = dyn_cast<BinaryOperator>(lhs))
                        {
                            switch (addOp->getOpcode())
                            {
                            // a = b * x, c = a + b -> c = b * (x + 1)
                            case Instruction::Mul: {
                                if (auto *constRhs = dyn_cast<ConstantInt>(addOp->getOperand(1)))
                                {
                                    if (rhs == addOp->getOperand(0))
                                    {
                                        Value *newInst = BinaryOperator::Create(
                                            Instruction::Mul, addOp->getOperand(0),
                                            ConstantInt::get(constRhs->getType(), constRhs->getValue() + 1), "", binOp);
                                        binOp->replaceAllUsesWith(newInst);
                                        ++strengthReductionTimes;
                                    }
                                }
                                break;
                            }
                            // 位移改为乘法之后再进行强度削减
                            case Instruction::Shl: {
                                if (auto *constRhs = dyn_cast<ConstantInt>(addOp->getOperand(1)))
                                {
                                    if (rhs == addOp->getOperand(0))
                                    {
                                        int shift = constRhs->getValue().getLimitedValue();
                                        int mul = 1;
                                        for (int i = 0; i < shift; ++i)
                                            mul *= 2;
                                        Value *newInst = BinaryOperator::Create(
                                            Instruction::Mul, addOp->getOperand(0),
                                            ConstantInt::get(constRhs->getType(), mul + 1), "", binOp);
                                        binOp->replaceAllUsesWith(newInst);
                                        ++strengthReductionTimes;
                                    }
                                    else if (lhs == addOp->getOperand(0))
                                    {
                                        int shift = constRhs->getValue().getLimitedValue();
                                        int mul = 1;
                                        for (int i = 0; i < shift; ++i)
                                            mul *= 2;
                                        Value *newInst = BinaryOperator::Create(
                                            Instruction::Mul, addOp->getOperand(0),
                                            ConstantInt::get(constRhs->getType(), mul + 1), "", binOp);
                                        binOp->replaceAllUsesWith(newInst);
                                        ++strengthReductionTimes;
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }
    }

    mOut << "StrengthReduction running...\n\rTo reduce " << strengthReductionTimes << " instructions\n\r";
    return PreservedAnalyses::all();
}
