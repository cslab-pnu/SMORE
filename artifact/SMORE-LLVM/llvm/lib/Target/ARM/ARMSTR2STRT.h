#ifndef ARM_STR2STRT
#define ARM_STR2STRT

#include "ARMInstrumentor.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm {

  struct ARMSTR2STRT
      : public MachineFunctionPass, ARMInstrumentor {
    // pass identifier variable
    static char ID;

    ARMSTR2STRT();

    virtual StringRef getPassName() const override;

    virtual bool runOnMachineFunction(MachineFunction & MF) override;
  };

  FunctionPass *createARMSTR2STRT(void);
}

#endif