#include "CBIInstrument.h"

using namespace llvm;

namespace instrument
{

  const auto PASS_NAME = "CBIInstrument";
  const auto PASS_DESC = "Instrumentation for CBI";
  const auto CBI_BRANCH_FUNCTION_NAME = "__cbi_branch__";
  const auto CBI_RETURN_FUNCTION_NAME = "__cbi_return__";

  /**
   * @brief Instrument a BranchInst with calls to __cbi_branch__
   *
   * @param M Module containing Branch
   * @param Branch A conditional Branch Instruction
   * @param Line Line number of Branch
   * @param Col Coulmn number of Branch
   */
  void instrumentBranch(Module *M, BranchInst *Branch, int Line, int Col);

  /**
   * @brief Instrument the return value of CallInst using calls to __cbi_return__
   *
   * @param M Module containing Call
   * @param Call A Call instruction that returns an Int32.
   * @param Line Line number of the Call
   * @param Col Column number of the Call
   */
  void instrumentReturn(Module *M, CallInst *Call, int Line, int Col);

  bool CBIInstrument::runOnFunction(Function &F)
  {
    auto FunctionName = F.getName().str();
    outs() << "Running " << PASS_DESC << " on function " << FunctionName << "\n";

    LLVMContext &Context = F.getContext();
    Module *M = F.getParent();

    Type *VoidType = Type::getVoidTy(Context);
    Type *Int32Type = Type::getInt32Ty(Context);
    Type *BoolType = Type::getInt1Ty(Context);

    M->getOrInsertFunction(CBI_BRANCH_FUNCTION_NAME, VoidType, Int32Type,
                           Int32Type, BoolType);

    M->getOrInsertFunction(CBI_RETURN_FUNCTION_NAME, VoidType, Int32Type,
                           Int32Type, Int32Type);

    for (inst_iterator Iter = inst_begin(F), E = inst_end(F); Iter != E; ++Iter)
    {
      Instruction &Inst = (*Iter);
      llvm::DebugLoc DebugLoc = Inst.getDebugLoc();
      if (!DebugLoc)
      {
        // Skip Instruction if it doesn't have debug information.
        continue;
      }

      int Line = DebugLoc.getLine();
      int Col = DebugLoc.getCol();

      /**
       * TODO: Add code to check the type of instruction
       * and call appropriate instrumentation function.
       *
       * If Inst is a Branch Instruction then
       *   use instrumentBranch
       * Else if it is a Call returning an int then
       *   use instrumentReturn
       * @param Branch
       */

      // do dyn cast of Inst and then check to see if the branch is conditional - using isConditional function that can be dereferenced from branch - use this to pass in the Branch argument for instrument branch (cond)

      if (auto *instruction = dyn_cast<BranchInst>(&Inst))
      {
        if (instruction->isConditional())
        {
          instrumentBranch(M, instruction, Line, Col);
        }
      }
      else if (auto *call = dyn_cast<CallInst>(&Inst))
      {
        // check call for getType integer to make sure the integer is 32 bits long
        if (call->getType()->isIntegerTy(32))
        {
          instrumentReturn(M, call, Line, Col);
        }
      }

      // if the call instruction has a return type of int 32, call instrumentReturn
    }
    return true;
  }

  /**
   * Implement instrumentation for the branch scheme of CBI. (Lab 9)
   */
  void instrumentBranch(Module *M, BranchInst *Branch, int Line, int Col)
  {
    auto &Context = M->getContext();
    auto Int32Type = Type::getInt32Ty(Context);

    /**
     * TODO: Add code to instrument the Branch Instruction.
     */
    auto LineVal = ConstantInt::get(Int32Type, Line);
    auto ColVal = ConstantInt::get(Int32Type, Col);
    auto cond = Branch->getCondition();

    // add in condition as argument from dereferenced branch argument
    std::vector<Value *> Args = {LineVal, ColVal, cond};

    auto *branchFunction = M->getFunction(CBI_BRANCH_FUNCTION_NAME);
    CallInst::Create(branchFunction, Args, "", Branch);
  }

  /**
   * Implement instrumentation for the return scheme of CBI. (Lab 9)
   */
  void instrumentReturn(Module *M, CallInst *Call, int Line, int Col)
  {
    auto &Context = M->getContext();
    auto Int32Type = Type::getInt32Ty(Context);

    /**
     * TODO: Add code to instrument the Call Instruction.
     *
     * Note: CallInst::Create(.) follows Insert Before semantics.
     */
    auto LineVal = ConstantInt::get(Int32Type, Line);
    auto ColVal = ConstantInt::get(Int32Type, Col);

    // add call as argument - don't need to dereference
    std::vector<Value *> Args = {LineVal, ColVal, Call};

    auto *returnFunction = M->getFunction(CBI_RETURN_FUNCTION_NAME);
    CallInst::Create(returnFunction, Args, "", Call->getNextNode());
  }

  char CBIInstrument::ID = 1;
  static RegisterPass<CBIInstrument> X(PASS_NAME, PASS_DESC, false, false);

} // namespace instrument
