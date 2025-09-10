#include "ARMInsertRedzone.h"
#include "ARM.h"
#include "ARMTargetMachine.h"
#include "ARMBaseInstrInfo.h"
#include "ARMBaseRegisterInfo.h"
#include "ARMConstantPoolValue.h"
#include "ARMMachineFunctionInfo.h"
#include "ARMSubtarget.h"
#include "MCTargetDesc/ARMBaseInfo.h"
#include "MCTargetDesc/ARMMCTargetDesc.h"
#include "Utils/ARMBaseInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCRegisterInfo.h"

using namespace llvm;

char ARMInsertRedzone::ID = 0;

ARMInsertRedzone::ARMInsertRedzone() : MachineFunctionPass(ID){
    return;
}

StringRef ARMInsertRedzone::getPassName() const {
    return "ARM Insert Redzone in msp stack Pass";
}

std::pair<unsigned, unsigned> roundingThumb2Imm2(unsigned imm){
    std::pair<unsigned, unsigned> thumb2Imms;
    unsigned diff = 0;

    if(imm > 256 && imm <= 1024){
        diff = imm;
        imm = (imm - 256) % 4;
        diff = diff - imm;
    }else if(imm > 1024 && imm <= 4096){
        diff = imm;
        imm = (imm - 1024) % 16;
        diff = diff - imm;
    }

    thumb2Imms = std::make_pair(imm, diff);

    return thumb2Imms;
}

static void settingRedCanaryFrame(MachineFunction &MF,const DebugLoc &dl,
                                 const ARMBaseInstrInfo &TII,
                                 unsigned MIFlags = MachineInstr::NoFlags,
                                 ARMCC::CondCodes Pred = ARMCC::AL,
                                 unsigned PredReg = 0){
  SmallVector<MachineBasicBlock::iterator, 16> eraseTarget;

  for(MachineBasicBlock &MBB : MF){
    MachineBasicBlock::iterator mbbi = MBB.begin();
   
    bool store_deleted = false;
    for(;mbbi != MBB.end(); mbbi++){
        // stack check guard
        /*if(mbbi->getOpcode() == ARM::t2MOVi16 && mbbi->getOperand(1).isGlobal() && mbbi->getOperand(1).getGlobal()->getName() == "__stack_chk_guard") {
            eraseTarget.push_back(mbbi);
            eraseTarget.push_back(++mbbi);
            eraseTarget.push_back(++mbbi);
        }

        if(mbbi->getOpcode() == ARM::t2STRi12 && mbbi->getOperand(1).isFI() && mbbi->getOperand(1).getIndex() == 0 && mbbi->getParent() == &*MF.begin()){
            eraseTarget.push_back(mbbi);
            store_deleted = true;
        }if(mbbi->getOpcode() == ARM::tSTRspi && mbbi->getOperand(2).isImm() && mbbi->getOperand(2).getImm() % 8 == 0 && !store_deleted && mbbi->getParent() == &*MF.begin()){
            eraseTarget.push_back(mbbi);
            store_deleted = true;
        }if ((mbbi->getOpcode() == ARM::t2STRi8 && mbbi->getOperand(1).getReg() == ARM::R7 && mbbi->getOperand(2).isImm() && mbbi->getOperand(2).getImm() == -56) && !store_deleted && mbbi->getParent() == &*MF.begin()) {
            eraseTarget.push_back(mbbi);
            store_deleted = true;
        }*/

        if(mbbi->getOpcode() == ARM::tSUBspi || mbbi->getOpcode() == ARM::t2SUBspImm || mbbi->getOpcode() == ARM::t2SUBspImm12){
            if(mbbi->getOperand(0).getReg() == ARM::SP){
                if(mbbi->getFlags() == MachineInstr::FrameSetup){
                    if(MF.getName() == "set_mode" || MF.getName() == "periph_apb_clk") continue;
                    auto checkPos = mbbi;
                    bool canMove = true;
                    for(int i = 0; i<2; i++){
                        if(std::next(checkPos) == MBB.end()){
                            canMove = false;
                        }
                        ++checkPos;
                    }

                    if(canMove){
                        mbbi++;
                        mbbi++;
                        if(mbbi->getOpcode() == ARM::tPUSH || mbbi->getOpcode() == ARM::t2STMDB_UPD){
                            mbbi--;
                            mbbi--;
                            unsigned imm = mbbi->getOperand(2).getImm();
                            std::string asmStr;

                            for(MachineBasicBlock &AdjustMBB : MF) {
                                for(MachineInstr &AdjustMI : AdjustMBB) {
                                    if(AdjustMI.getOpcode() == ARM::t2ADDri) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            errs() << "destReg: " << destReg << "\n";
                                                unsigned newImm = 40 - imm*4;
                                           
                                                if(destReg == 13){
                                                    asmStr = "add lr, r7, #" +std::to_string(newImm);
                                                }else{
                                                    asmStr = "add r"+ std::to_string(destReg-73) + ", r7, #" +std::to_string(newImm);
                                                }
                                            
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::t2STRDi8) {
                                        if(AdjustMI.getOperand(2).isReg() && AdjustMI.getOperand(2).getReg() == ARM::R7) {
                                            unsigned destReg1 = AdjustMI.getOperand(0).getReg();
                                            unsigned destReg2 = AdjustMI.getOperand(1).getReg();
                                            
                                            if(AdjustMI.getOperand(3).getImm() >= 0){ 
                                                unsigned newImm = 40 - AdjustMI.getOperand(3).getImm();
                                            
                                                std::string asmStr = "strd r"+ std::to_string(destReg1-73) + ", r"+
                                                                std::to_string(destReg2-73)+", [r7, #" + 
                                                                std::to_string(newImm)+"]";
                                            
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                            }
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::t2STRi12 || AdjustMI.getOpcode() == ARM::t2STRi8) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            if(AdjustMI.getOperand(2).getImm() >= 0){ 
                                                unsigned newImm = 40 - AdjustMI.getOperand(2).getImm();
                                            
                                                if(destReg == 13){
                                                    asmStr = "str lr, [r7, #" +std::to_string(newImm)+"]";

                                                }else{
                                                    asmStr = "str r"+ std::to_string(destReg-73) + ", [r7, #" +std::to_string(newImm)+"]";
                                                }
                                            
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                            
                                            }
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::tSTRi) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            
                                            if(AdjustMI.getOperand(2).getImm() >= 0){ 
                                                unsigned newImm = 40 - AdjustMI.getOperand(2).getImm()*4;
                                            
                                                if(destReg == 13){
                                                    asmStr = "str lr, [r7, #" +std::to_string(newImm)+"]";
                                                }else{
                                                    asmStr = "str r"+ std::to_string(destReg-73) + ", [r7, #" +std::to_string(newImm)+"]";
                                                }
                                            
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                            }
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::t2STRBi8) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            unsigned newImm = 40 - AdjustMI.getOperand(2).getImm();
                                            
                                            if(AdjustMI.getOperand(2).getImm() >= 0){ 
                                                if(destReg == 13){
                                                    asmStr = "strb lr, [r7, #" + std::to_string(newImm)+"]";
                                                }else{
                                                    asmStr = "strb r"+ std::to_string(destReg-73) + ", [r7, #" + std::to_string(newImm)+"]";
                                                }
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                            }
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::tSTRBi) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            if(AdjustMI.getOperand(2).getImm() >= 0){ 

                                                unsigned newImm = 40 - AdjustMI.getOperand(2).getImm()*4;
                                           
                                                if(destReg == 13){
                                                    asmStr = "strb lr, [r7, #" + std::to_string(newImm)+"]";
                                                }else{
                                                    asmStr = "strb r"+ std::to_string(destReg-73) + ", [r7, #" + std::to_string(newImm)+"]";
                                                }
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                            }
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::t2LDRi12 || AdjustMI.getOpcode() == ARM::t2LDRi8) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            
                                            if(AdjustMI.getOperand(2).getImm() >= 0){ 

                                                unsigned newImm = 40 - AdjustMI.getOperand(2).getImm();
                                            
                                                if(destReg == 13){
                                                    asmStr = "ldr lr, [r7, #" + std::to_string(newImm)+"]";
                                                }else{
                                                    asmStr = "ldr r"+ std::to_string(destReg-73) + ", [r7, #" + std::to_string(newImm)+"]";
                                                }
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                                
                                            }
                                        }
                                    }else if(AdjustMI.getOpcode() == ARM::tLDRi) {
                                        if(AdjustMI.getOperand(1).isReg() && AdjustMI.getOperand(1).getReg() == ARM::R7) {
                                            unsigned destReg = AdjustMI.getOperand(0).getReg();
                                            
                                            if(AdjustMI.getOperand(2).getImm() >= 0){ 
                                                unsigned newImm = 40 - AdjustMI.getOperand(2).getImm()*4;
                                            
                                                if(destReg == 13){
                                                    asmStr = "ldr lr, [r7, #" + std::to_string(newImm)+"]";
                                                }else{
                                                    asmStr = "ldr r"+ std::to_string(destReg-73) + ", [r7, #" + std::to_string(newImm)+"]";
                                                }
                                                BuildMI(AdjustMBB, AdjustMI, dl, TII.get(ARM::INLINEASM))
                                                    .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                                                    .addImm(1)
                                                    .addExternalSymbol("");
                                            
                                                eraseTarget.push_back(AdjustMI.getIterator());
                                        }
                                    }
                                }
                            }
                        }

                            imm = 8;
                            if (mbbi->getOpcode() == ARM::tSUBspi) {
                                mbbi->getOperand(2).setImm(imm);
                            } else if (mbbi->getOpcode() == ARM::t2SUBspImm || mbbi->getOpcode() == ARM::t2SUBspImm12) {
                                mbbi->getOperand(2).setImm(imm);
                            }
                            mbbi++;
                            mbbi++;

                            asmStr = "stmia sp, {r4, r5, r6, r7, r8, lr}\nbl resetRedzone\nldmia sp, {r4, r5, r6, r7, r8, lr}\n";
                            StringRef asmString = asmStr;
                            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName(asmString))
                                .addImm(1)
                                .addExternalSymbol("");
                        }else{
                            mbbi--;
                            mbbi--;
                            eraseTarget.push_back(mbbi);
                        }
                    }else{
                        eraseTarget.push_back(mbbi);
                    }
                }/*else{
                    eraseTarget.push_back(mbbi);
                }*/
            }
        }else if(mbbi->getOpcode() == ARM::tADDspi || mbbi->getOpcode() == ARM::t2ADDspImm || mbbi->getOpcode() == ARM::t2ADDspImm12){
            if(mbbi->getOperand(0).getReg() == ARM::SP){
                if(mbbi->getFlags() == MachineInstr::FrameDestroy){
                    if(MF.getName() == "set_mode" || MF.getName() == "periph_apb_clk") continue;
                    auto checkPos = mbbi;
                    bool canMove = true;
                    for(int i = 0; i<2; i++){
                        if(checkPos == MBB.begin()){
                            canMove = false;
                            break;
                        }
                        --checkPos;
                    }


                    if(canMove){
                        mbbi--;
                        if(mbbi->isDebugInstr()) mbbi--;
                        if(mbbi->getOpcode() == ARM::tPOP || mbbi->getOpcode() == ARM::t2LDMIA_RET || mbbi->getOpcode() == ARM::t2LDMIA_UPD || mbbi->getOpcode() == ARM::tPOP_RET){
                            mbbi++;
                            if(mbbi->isDebugInstr()) mbbi++;

                            unsigned imm = mbbi->getOperand(2).getImm();
                            imm = 8;
                            if (mbbi->getOpcode() == ARM::tADDspi) {
                                mbbi->getOperand(2).setImm(imm);
                            } else if (mbbi->getOpcode() == ARM::t2ADDspImm || mbbi->getOpcode() == ARM::t2ADDspImm12) {
                                mbbi->getOperand(2).setImm(imm);
                            }
                            mbbi++;

                            std::string asmStr = "stmdb sp, {r4, r5, r6, r7, r8, lr}\nbl resetRedzone\nldmdb sp, {r4, r5, r6, r7, r8, lr}\n";
                            StringRef asmString = asmStr;
                            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                                .addExternalSymbol(MF.createExternalSymbolName(asmString))
                                .addImm(1)
                                .addExternalSymbol("");
                        }else{
                            mbbi++;
                            if(mbbi->isDebugInstr()) mbbi++;
                            eraseTarget.push_back(mbbi);
                        }
                    }else{
                        eraseTarget.push_back(mbbi);
                    }
                }/*else{
                    eraseTarget.push_back(mbbi);
                }*/
            }
        }
        else if(mbbi->getOpcode() == ARM::tMOVr && mbbi->getOperand(0).getReg() == ARM::SP){

            int i=0;
            bool move = false;
            mbbi++;
            while (mbbi->isDebugInstr()) { i++; mbbi++; }
            /*if(mbbi->getFlags() != MachineInstr::FrameDestroy){
                mbbi++;
                mbbi++;
                move = true;
            }*/
            if(mbbi->getOpcode() == ARM::t2SUBspImm || mbbi->getOpcode() == ARM::t2SUBspImm12 || mbbi->getOpcode() == ARM::tSUBspi){
                mbbi++;
            }/*else if(move){
                mbbi--;
                mbbi--;
            }*/
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("mov r9, sp"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("bic.w r9, r9, #31"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("mov sp, r9"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("movw r9, #0x400"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("movt r9, #0x2000"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("stm r9, {r4, r5, r6, r7, r8, lr}"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::tBL))
                .add(predOps(ARMCC::AL))
                .addExternalSymbol("resetRedzone")
                .setMIFlags(MIFlags);
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("movw r9, #0x400"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("movt r9, #0x2000"))
                .addImm(1)
                .addExternalSymbol("");
            BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                .addExternalSymbol(MF.createExternalSymbolName("ldm r9, {r4, r5, r6, r7, r8, lr}"))
                .addImm(1)
                .addExternalSymbol("");
            
            mbbi--;

            if(mbbi->getOpcode() == ARM::t2SUBspImm || mbbi->getOpcode() == ARM::t2SUBspImm12 || mbbi->getOpcode() == ARM::tSUBspi){
                for (int j=0; j<i; j++) mbbi--;
            }
        }
        else if(mbbi->getOpcode() == ARM::t2SUBri && mbbi->getFlags() == MachineInstr::FrameDestroy && mbbi->getOperand(1).getReg() == ARM::R7){
            unsigned src = mbbi->getOperand(0).getReg();
            unsigned base = mbbi->getOperand(1).getReg();
            //unsigned imm = mbbi->getOperand(2).getImm();

            unsigned frameSize = MF.getFrameInfo().getStackSize();

            unsigned imm = frameSize - 8;
            int addCnt = 0;
            
            while(imm > 4096){
                addCnt++;
                imm -= 4096;
            }

            std::pair<unsigned, unsigned> thumb2Imm = roundingThumb2Imm2(imm);
            imm = thumb2Imm.first;
            unsigned diff = thumb2Imm.second;

            bool alreadyInst = false;
            while(addCnt > 0){
                if (!alreadyInst) {
                    BuildMI(MBB, mbbi, dl, TII.get(ARM::t2SUBri))
                        .addReg(src, RegState::Kill)
                        .addReg(base, RegState::Kill)
                        .addImm(4096)
                        .add(predOps(ARMCC::AL))
                        .add(condCodeOp())
                        .setMIFlag(MachineInstr::FrameDestroy);
                    alreadyInst = true;
                } else{
                     BuildMI(MBB, mbbi, dl, TII.get(ARM::t2SUBri))
                        .addReg(src, RegState::Kill)
                        .addReg(src, RegState::Define)
                        .addImm(4096)
                        .add(predOps(ARMCC::AL))
                        .add(condCodeOp());
                }
                addCnt--;
            }
            
            if(diff != 0 && imm == 0){
                BuildMI(MBB, mbbi, dl, TII.get(ARM::t2SUBri))
                    .addReg(src, RegState::Kill)
                    .addReg(base, RegState::Kill)
                    .addImm(diff)
                    .add(predOps(ARMCC::AL))
                    .add(condCodeOp());
            } else if (imm != 0 && diff == 0) {
                BuildMI(MBB, mbbi, dl, TII.get(ARM::t2SUBri))
                    .addReg(src, RegState::Kill)
                    .addReg(base, RegState::Define)
                    .addImm(imm)
                    .add(predOps(ARMCC::AL))
                    .add(condCodeOp());
            } else {
                BuildMI(MBB, mbbi, dl, TII.get(ARM::t2SUBri))
                    .addReg(src, RegState::Kill)
                    .addReg(base, RegState::Define)
                    .addImm(diff)
                    .add(predOps(ARMCC::AL))
                    .add(condCodeOp());
                BuildMI(MBB, mbbi, dl, TII.get(ARM::t2SUBri))
                    .addReg(src, RegState::Kill)
                    .addReg(src, RegState::Define)
                    .addImm(imm)
                    .add(predOps(ARMCC::AL))
                    .add(condCodeOp());
            }

            eraseTarget.push_back(mbbi);

        }/*else if(mbbi->getOpcode() == ARM::t2STRBi8) {
            if(mbbi->getOperand(1).isReg() && mbbi->getOperand(1).getReg() == ARM::R7) {
                unsigned destReg = mbbi->getOperand(0).getReg();
                int imm = mbbi->getOperand(2).getImm();

                int newImm;
                if(imm < 0){
                    newImm = imm - 32;
                                            
                    std::string asmStr = "strb r"+ std::to_string(destReg-73) + ", [r7, #" + std::to_string(newImm)+"]";
                                            
                    BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                        .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                        .addImm(1)
                        .addExternalSymbol("");
                                            
                    eraseTarget.push_back(mbbi);
                
                }
            }
        }else if(mbbi->getOpcode() == ARM::t2LDRBi8) {
            if(mbbi->getOperand(1).isReg() && mbbi->getOperand(1).getReg() == ARM::R7) {
                unsigned destReg = mbbi->getOperand(0).getReg();
                int imm = mbbi->getOperand(2).getImm();

                int newImm;
                if(imm < 0){
                    newImm = imm - 32;

                    std::string asmStr;
                    if(destReg == 13){
                        asmStr = "ldrb lr, [r7, #" + std::to_string(newImm)+"]";
                    }else{                    
                        asmStr = "ldrb r"+ std::to_string(destReg-73) + ", [r7, #" + std::to_string(newImm)+"]";
                    } 

                    BuildMI(MBB, mbbi, dl, TII.get(ARM::INLINEASM))
                        .addExternalSymbol(MF.createExternalSymbolName(asmStr))
                        .addImm(1)
                        .addExternalSymbol("");
                                            
                    eraseTarget.push_back(mbbi);
                }
            }
        }*/
    }
  }

  for(MachineBasicBlock::iterator target : eraseTarget){
        target->eraseFromParent();
  }
}

bool ARMInsertRedzone::runOnMachineFunction(MachineFunction &MF){
    ARMFunctionInfo *AFI = MF.getInfo<ARMFunctionInfo>();
    const ARMBaseInstrInfo &TII = *MF.getSubtarget<ARMSubtarget>().getInstrInfo();
    bool isARM = !AFI->isThumbFunction();
    DebugLoc dl ;
    Function &F = MF.getFunction();
    MachineFrameInfo &MFI = MF.getFrameInfo();

	/*
    if(F.getName() == "dummy_handler_default" || F.getName() == "nmi_handler" || F.getName() == "bus_fault_default" || 
        F.getName() == "usage_fault_default" || F.getName() == "debug_mon_default" || F.getName() == "core_panic" ||
        F.getName() == "pm_reboot" || F.getName() == "pm_off" || F.getName() == "_Exit" || F.getName() == "_start" ||
        F.getName() == "_exit" || F.getName() == "abort" || F.getName() == "__assert"|| F.getName() == "__assert_func" ||
        F.getName() == "__eprintf" || F.getName() == "exit" || F.getName() == "quick_exit" || F.getName() == "__chk_fail" ||
        F.getName() == "__stack_chk_fail_local" || F.getName() == "__stack_chk_fail" || F.getName() == "__memmove_chk" ||
        F.getName() == "__memset_chk" || F.getName() == "_kill_shared" || F.getName() == "_kill" || F.getName() == "_main" ||
        F.getName() == "reset_handler_default") {

        errs() << "Skip " << F.getName() << "function inserting code(end of program)\n";
        return true;
    }*/
	if(F.getName() == "reset_handler_default" || F.getName() == "hard_fault_handler")
		return true;

    settingRedCanaryFrame(MF, dl, TII);
    if (F.getName() == "psa_algorithm_dispatch_hash_update") return true;
    //insertDummyHandler(MF, dl, TII);

    return true;
}

namespace llvm{
    FunctionPass *createARMInsertRedzone(void){
        return new ARMInsertRedzone();
    }
}
