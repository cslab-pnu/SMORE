#ifndef ARMINSERTREDZONE
#define ARMINSERTREDZONE

#include "llvm/CodeGen/MachineFunctionPass.h"

namespace llvm{
    struct ARMInsertRedzone : public MachineFunctionPass{
        static char ID;

        ARMInsertRedzone();
        virtual StringRef getPassName() const override;
        virtual bool runOnMachineFunction(MachineFunction &MF) override;
    };

    FunctionPass *createARMInsertRedzone(void);
}

#endif