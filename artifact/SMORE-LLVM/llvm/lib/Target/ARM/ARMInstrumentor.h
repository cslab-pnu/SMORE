//===- ARMSilhouetteInstrumentor - A helper class for instrumentation -----===//
//
//         Protecting Control Flow of Real-time OS applications
//              Copyright (c) 2019-2020, University of Rochester
//
// Part of the Silhouette Project, under the Apache License v2.0 with
// LLVM Exceptions.
// See LICENSE.txt in the top-level directory for license information.
//
//===----------------------------------------------------------------------===//
//
// This file defines interfaces of the ARMSilhouetteInstrumentor class.
//
//===----------------------------------------------------------------------===//
//

#ifndef ARM_INSTRUMENTOR
#define ARM_INSTRUMENTOR

#include "ARMBaseInstrInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

#include <deque>
#include <vector>

namespace llvm {
  //====================================================================
  // Static inline functions.
  //====================================================================

  //
  // Function: getFunctionCodeSize()
  //
  // Description:
  //   This function computes the code size of a machine function.
  //
  // Input:
  //   MF - A reference to the target machine function.
  //
  // Return value:
  //   The size (in bytes) of the machine function.
  //
  static inline unsigned long getFunctionCodeSize(const MachineFunction & MF) {
    const TargetInstrInfo * TII = MF.getSubtarget().getInstrInfo();

    unsigned long CodeSize = 0ul;
    for (const MachineBasicBlock & MBB : MF) {
      for (const MachineInstr & MI : MBB) {
        CodeSize += TII->getInstSizeInBytes(MI);
      }
    }

    return CodeSize;
  }

  //
  // Function: addImmediateToRegister()
  //
  // Description:
  //   This function builds an ADD/SUB that adds an immediate to a register and
  //   puts the new instruction(s) at the end of a vector.
  //
  // Inputs:
  //   MI    - A reference to the instruction before which to insert new
  //           instructions.
  //   Reg   - The destination register.
  //   Imm   - The immediate to be added.
  //   Insts - A reference to a vector that contains new instructions.
  //
  static inline void
  addImmediateToRegister(MachineInstr & MI, Register Reg, int64_t Imm,
                         std::vector<MachineInstr *> & Insts) {
    assert((Imm > -4096 && Imm < 4096) && "Immediate too large!");
    assert((Reg != ARM::SP || Imm % 4 == 0) &&
            "Cannot add unaligned immediate to SP!");

    MachineFunction & MF = *MI.getMF();
    const TargetInstrInfo * TII = MF.getSubtarget().getInstrInfo();

    Register PredReg;
    ARMCC::CondCodes Pred = getInstrPredicate(MI, PredReg);

    unsigned addOpc = Imm < 0 ? ARM::t2SUBri12 : ARM::t2ADDri12;
    if (Reg == ARM::SP && Imm > -512 && Imm < 512) {
      addOpc = Imm < 0 ? ARM::tSUBspi : ARM::tADDspi;
      Imm >>= 2;
    }

    Insts.push_back(BuildMI(MF, MI.getDebugLoc(), TII->get(addOpc), Reg)
                    .addReg(Reg)
                    .addImm(Imm < 0 ? -Imm : Imm)
                    .add(predOps(Pred, PredReg)));
  }

  //
  // Function: subtractImmediateFromRegister()
  //
  // Description:
  //   This function builds a SUB/ADD that subtracts an immediate from a register
  //   and puts the new instruction(s) at the end of a vector.
  //
  // Inputs:
  //   MI    - A reference to the instruction before which to insert new
  //           instructions.
  //   Reg   - The destination register.
  //   Imm   - The immediate to be subtracted.
  //   Insts - A reference to a vector that contains new instructions.
  //
  static inline void
  subtractImmediateFromRegister(MachineInstr & MI, Register Reg, int64_t Imm,
                                std::vector<MachineInstr *> & Insts) {
    addImmediateToRegister(MI, Reg, -Imm, Insts);
  }

  //====================================================================
  // Class ARMSilhouetteInstrumentor.
  //====================================================================

  struct ARMInstrumentor {
    void insertInstBefore(MachineInstr & MI, MachineInstr * Inst);

    void insertInstAfter(MachineInstr & MI, MachineInstr * Inst);

    void insertInstsBefore(MachineInstr & MI, ArrayRef<MachineInstr *> Insts);

    void insertInstsAfter(MachineInstr & MI, ArrayRef<MachineInstr *> Insts);

    void removeInst(MachineInstr & MI);

    std::vector<Register> findFreeRegistersBefore(const MachineInstr & MI,
                                                  bool Thumb = false);

    std::vector<Register> findFreeRegistersAfter(const MachineInstr & MI,
                                                 bool Thumb = false);

  private:
    unsigned getITBlockSize(const MachineInstr & IT);
    MachineInstr * findIT(MachineInstr & MI, unsigned & distance);
    const MachineInstr * findIT(const MachineInstr & MI, unsigned & distance);
    std::deque<bool> decodeITMask(unsigned Mask);
    unsigned encodeITMask(std::deque<bool> DQMask);
  };
}

#endif