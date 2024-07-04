#include "ConstantFolding.hpp"

using namespace llvm;

PreservedAnalyses ConstantFolding::run(Module &mod, ModuleAnalysisManager &mam)
{
    int constFoldTimes = 0;

    for (auto &func : mod)
    {
        for (auto &bb : func)
        {
            for (auto &inst : bb)
            {
                // 判断当前指令是否是二元运算指令
                if (auto binOp = dyn_cast<BinaryOperator>(&inst))
                {
                    // 获取二元运算指令的左右操作数，并尝试转换为常整数
                    Value *lhs = binOp->getOperand(0);
                    Value *rhs = binOp->getOperand(1);
                    auto constLhs = dyn_cast<ConstantInt>(lhs);
                    auto constRhs = dyn_cast<ConstantInt>(rhs);
                    switch (binOp->getOpcode())
                    {
                    case Instruction::Add: {
                        // a = x + a -> a = a + x
                        if (constLhs && !constRhs)
                        {
                            binOp->setOperand(0, rhs);
                            binOp->setOperand(1, lhs);
                            constLhs = dyn_cast<ConstantInt>(rhs);
                            constRhs = dyn_cast<ConstantInt>(lhs);
                        }
                        // a = x + y -> a = z
                        else if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() + constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        else if (constRhs)
                        {
                            // a = a + 0 -> a = a
                            if (constRhs->isZero())
                            {
                                binOp->replaceAllUsesWith(lhs);
                                ++constFoldTimes;
                                break;
                            }
                            // a = a + x + y -> a = a + z
                            if (auto *lhsBinOp = dyn_cast<BinaryOperator>(lhs))
                            {
                                if (auto *lhsRhs = dyn_cast<ConstantInt>(lhsBinOp->getOperand(1)))
                                {
                                    if (lhsBinOp->getOpcode() == Instruction::Add)
                                    {
                                        binOp->setOperand(0, lhsBinOp->getOperand(0));
                                        binOp->setOperand(
                                            1, ConstantInt::getSigned(binOp->getType(), lhsRhs->getSExtValue() +
                                                                                            constRhs->getSExtValue()));
                                        ++constFoldTimes;
                                    }
                                    else if (lhsBinOp->getOpcode() == Instruction::Sub)
                                    {
                                        binOp->setOperand(0, lhsBinOp->getOperand(0));
                                        binOp->setOperand(
                                            1, ConstantInt::getSigned(binOp->getType(), 0 - lhsRhs->getSExtValue() +
                                                                                            constRhs->getSExtValue()));
                                        ++constFoldTimes;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case Instruction::Sub: {
                        // a = x - y -> a = z
                        if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() - constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        else if (constRhs)
                        {
                            // a = a - 0 -> a = a
                            if (constRhs->isZero())
                            {
                                binOp->replaceAllUsesWith(lhs);
                                ++constFoldTimes;
                                break;
                            }
                            // a = a - x - y -> a = a - z
                            if (auto *lhsBinOp = dyn_cast<BinaryOperator>(lhs))
                            {
                                if (auto *lhsRhs = dyn_cast<ConstantInt>(lhsBinOp->getOperand(1)))
                                {
                                    if (lhsBinOp->getOpcode() == Instruction::Add)
                                    {
                                        binOp->setOperand(0, lhsBinOp->getOperand(0));
                                        binOp->setOperand(
                                            1, ConstantInt::getSigned(binOp->getType(), lhsRhs->getSExtValue() -
                                                                                            constRhs->getSExtValue()));
                                        ++constFoldTimes;
                                    }
                                    else if (lhsBinOp->getOpcode() == Instruction::Sub)
                                    {
                                        binOp->setOperand(0, lhsBinOp->getOperand(0));
                                        binOp->setOperand(
                                            1, ConstantInt::getSigned(binOp->getType(), 0 - lhsRhs->getSExtValue() -
                                                                                            constRhs->getSExtValue()));
                                        ++constFoldTimes;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case Instruction::Mul: {
                        // a = x * a -> a = a * x
                        if (constLhs && !constRhs)
                        {
                            binOp->setOperand(0, rhs);
                            binOp->setOperand(1, lhs);
                            constLhs = dyn_cast<ConstantInt>(rhs);
                            constRhs = dyn_cast<ConstantInt>(lhs);
                        }
                        // a = x * y -> a = z
                        else if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() * constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = x * 0 -> a = 0
                        else if (constRhs && constRhs->isZero())
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(binOp->getType(), 0));
                            ++constFoldTimes;
                        }
                        break;
                    }
                    case Instruction::UDiv:
                    case Instruction::SDiv: {
                        // a = x / y -> a = z
                        if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() / constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = b / 1 -> a = b
                        else if (constRhs && constRhs->isOne())
                        {
                            binOp->replaceAllUsesWith(lhs);
                            ++constFoldTimes;
                        }
                        // a = b / -1 -> a = -b
                        else if (constRhs && constRhs->isMinusOne())
                        {
                            binOp->replaceAllUsesWith(
                                ConstantInt::getSigned(binOp->getType(), -constLhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = 0 / x -> a = 0
                        else if (constLhs && constLhs->isZero())
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(binOp->getType(), 0));
                            ++constFoldTimes;
                        }
                        break;
                    }
                    case Instruction::Shl: {
                        // a = x << y -> a = z
                        if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() << constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = 0 << x -> a = 0
                        else if (constLhs && constLhs->isZero())
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(binOp->getType(), 0));
                            ++constFoldTimes;
                        }
                        // a = x << 0 -> a = x
                        else if (constRhs && constRhs->isZero())
                        {
                            binOp->replaceAllUsesWith(lhs);
                            ++constFoldTimes;
                        }
                        break;
                    }
                    case Instruction::LShr:
                    case Instruction::AShr: {
                        // a = x >> y -> a = z
                        if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() >> constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = 0 >> x -> a = 0
                        else if (constLhs && constLhs->isZero())
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(binOp->getType(), 0));
                            ++constFoldTimes;
                        }
                        // a = x >> 0 -> a = x
                        else if (constRhs && constRhs->isZero())
                        {
                            binOp->replaceAllUsesWith(lhs);
                            ++constFoldTimes;
                        }
                        break;
                    }
                    case Instruction::And: {
                        // a = x & a -> a = a & x
                        if (constLhs && !constRhs)
                        {
                            binOp->setOperand(0, rhs);
                            binOp->setOperand(1, lhs);
                            constLhs = dyn_cast<ConstantInt>(rhs);
                            constRhs = dyn_cast<ConstantInt>(lhs);
                        }
                        // a = x & y -> a = z
                        else if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() & constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = x & 0 -> a = 0
                        else if (constRhs && constRhs->isZero())
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(binOp->getType(), 0));
                            ++constFoldTimes;
                        }
                        // a = x & -1 -> a = x
                        else if (constRhs && constRhs->isAllOnesValue())
                        {
                            binOp->replaceAllUsesWith(lhs);
                            ++constFoldTimes;
                        }
                        break;
                    }
                    case Instruction::Or: {
                        // a = x | a -> a = a | x
                        if (constLhs && !constRhs)
                        {
                            binOp->setOperand(0, rhs);
                            binOp->setOperand(1, lhs);
                            constLhs = dyn_cast<ConstantInt>(rhs);
                            constRhs = dyn_cast<ConstantInt>(lhs);
                        }
                        // a = x | y -> a = z
                        else if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() | constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = x | 0 -> a = x
                        else if (constRhs && constRhs->isZero())
                        {
                            binOp->replaceAllUsesWith(lhs);
                            ++constFoldTimes;
                        }
                        // a = x | -1 -> a = -1
                        else if (constRhs && constRhs->isAllOnesValue())
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(binOp->getType(), -1));
                            ++constFoldTimes;
                        }
                        break;
                    }
                    case Instruction::Xor: {
                        // a = x ^ a -> a = a ^ x
                        if (constLhs && !constRhs)
                        {
                            binOp->setOperand(0, rhs);
                            binOp->setOperand(1, lhs);
                            constLhs = dyn_cast<ConstantInt>(rhs);
                            constRhs = dyn_cast<ConstantInt>(lhs);
                        }
                        // a = x ^ y -> a = z
                        else if (constLhs && constRhs)
                        {
                            binOp->replaceAllUsesWith(ConstantInt::getSigned(
                                binOp->getType(), constLhs->getSExtValue() ^ constRhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        // a = x ^ 0 -> a = x
                        else if (constRhs && constRhs->isZero())
                        {
                            binOp->replaceAllUsesWith(lhs);
                            ++constFoldTimes;
                        }
                        // a = x ^ -1 -> a = ~x
                        else if (constRhs && constRhs->isAllOnesValue())
                        {
                            binOp->replaceAllUsesWith(
                                ConstantInt::getSigned(binOp->getType(), ~constLhs->getSExtValue()));
                            ++constFoldTimes;
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
                else if (auto icmp = dyn_cast<ICmpInst>(&inst))
                {
                    Value *lhs = icmp->getOperand(0);
                    Value *rhs = icmp->getOperand(1);
                    auto constLhs = dyn_cast<ConstantInt>(lhs);
                    auto constRhs = dyn_cast<ConstantInt>(rhs);
                    if (constLhs && constRhs)
                    {
                        switch (icmp->getPredicate())
                        {
                        case CmpInst::ICMP_EQ:
                            icmp->replaceAllUsesWith(ConstantInt::getFalse(mod.getContext()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_NE:
                            icmp->replaceAllUsesWith(ConstantInt::getTrue(mod.getContext()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_UGT:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getZExtValue() > constRhs->getZExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_UGE:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getZExtValue() >= constRhs->getZExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_ULT:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getZExtValue() < constRhs->getZExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_ULE:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getZExtValue() <= constRhs->getZExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_SGT:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getSExtValue() > constRhs->getSExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_SGE:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getSExtValue() >= constRhs->getSExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_SLT:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getSExtValue() < constRhs->getSExtValue()));
                            ++constFoldTimes;
                            break;
                        case CmpInst::ICMP_SLE:
                            icmp->replaceAllUsesWith(ConstantInt::getSigned(
                                icmp->getType(), constLhs->getSExtValue() <= constRhs->getSExtValue()));
                            ++constFoldTimes;
                            break;
                        default:
                            break;
                        }
                    }
                }
                else if (auto br = dyn_cast<BranchInst>(&inst))
                {
                    if (br->isConditional())
                    {
                        if (auto constCond = dyn_cast<ConstantInt>(br->getCondition()))
                        {
                            BranchInst::Create(br->getSuccessor(constCond->isZero() ? 1 : 0), br);
                            ++constFoldTimes;
                            br->eraseFromParent();
                            break;
                        }
                    }
                }
            }
        }
    }

    mOut << "ConstantFolding running...\n\rTo eliminate " << constFoldTimes << " instructions\n\r";
    return PreservedAnalyses::all();
}
