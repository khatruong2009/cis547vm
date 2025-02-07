#include "Instrument.h"
#include "Utils.h"

using namespace llvm;

namespace instrument
{

  const auto PASS_NAME = "StaticAnalysisPass";
  const auto PASS_DESC = "Static Analysis Pass";

  bool Instrument::runOnFunction(Function &F)
  {
    auto FunctionName = F.getName().str();
    outs() << "Running " << PASS_DESC << " on function " << FunctionName << "\n";

    outs() << "Locating Instructions\n";
    for (inst_iterator Iter = inst_begin(F), E = inst_end(F); Iter != E; ++Iter)
    {
      Instruction &Inst = (*Iter);
      llvm::DebugLoc DebugLoc = Inst.getDebugLoc();
      // llvm::AddrSpaceCastInst::BinaryOps BinaryOps = Inst.getOpcode();
      if (!DebugLoc)
      {
        // Skip Instruction if it doesn't have debug information.
        continue;
      }

      int Line = DebugLoc.getLine();
      int Col = DebugLoc.getCol();
      outs() << Line << ", " << Col << "\n";

      /**
       * TODO: Add code to check if the instruction is a BinaryOperator and if so,
       * print the information about its location and operands as specified in the
       * Lab document.
       */

      // auto *binary = dyn_cast<BinaryOperator>(*Iter);
      // auto *binary = dyn_cast<BinaryOps>(&Inst);

      if (auto *binary = dyn_cast<BinaryOperator>(&Inst))
      {
        char c = getBinOpSymbol(binary->getOpcode());
        outs() << getBinOpName(c) << " on Line " << Line << ", Column " << Col << " with first operand " << variable(binary->getOperand(0)) << " and second operand " << variable(binary->getOperand(1)) << "\n";
        // outs() << "\n";
      }
    }
    return false;
  }

  char Instrument::ID = 1;
  static RegisterPass<Instrument> X(PASS_NAME, PASS_NAME, false, false);

} // namespace instrument
