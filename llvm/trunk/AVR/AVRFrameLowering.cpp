//===-- AVRFrameLowering.cpp - AVR Frame Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the AVR implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "AVRFrameLowering.h"
#include "AVR.h"
#include "AVRInstrInfo.h"
#include "AVRMachineFunctionInfo.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

AVRFrameLowering::AVRFrameLowering() :
  TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 1, -2) {}

bool
AVRFrameLowering::canSimplifyCallFramePseudos(const MachineFunction &MF) const
{
  // Always simplify call frame pseudo instructions, even when
  // hasReservedCallFrame is false.
  return true;
}

bool AVRFrameLowering::hasReservedCallFrame(const MachineFunction &MF) const
{
  // Reserve call frame memory in function prologue when Y is reserved to be
  // the frame pointer and when the function does not contain variable sized
  // objects.
  //:TODO: if maxcallframesize is >63 std will fail folding the offset.
  return hasFP(MF) && !MF.getFrameInfo()->hasVarSizedObjects();
}

void AVRFrameLowering::emitPrologue(MachineFunction &MF) const
{
  MachineBasicBlock &MBB = MF.front();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  CallingConv::ID CallConv = MF.getFunction()->getCallingConv();
  DebugLoc dl = (MBBI != MBB.end()) ? MBBI->getDebugLoc() : DebugLoc();
  const AVRInstrInfo &TII =
    *static_cast<const AVRInstrInfo *>(MF.getTarget().getInstrInfo());

  // Interrupt handlers re-enable interrupts in function entry.
  if (CallConv == CallingConv::AVR_INTR)
  {
    BuildMI(MBB, MBBI, dl, TII.get(AVR::SEI))
      .setMIFlag(MachineInstr::FrameSetup);
  }

  // Emit special prologue code to save R1, R0 and SREG in interrupt/signal
  // handlers before saving any other registers.
  if (CallConv == CallingConv::AVR_INTR || CallConv == CallingConv::AVR_SIGNAL)
  {
    BuildMI(MBB, MBBI, dl, TII.get(AVR::PUSHWRr))
      .addReg(AVR::R1R0, RegState::Kill)
      .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, MBBI, dl, TII.get(AVR::INRdA), AVR::R0)
      .addImm(0x3f)
      .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, MBBI, dl, TII.get(AVR::PUSHRr))
      .addReg(AVR::R0, RegState::Kill)
      .setMIFlag(MachineInstr::FrameSetup);
    //:TODO: clr __zero_reg__
  }

  // Early exit if the frame pointer is not needed in this function.
  if (!hasFP(MF))
  {
    return;
  }

  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const AVRMachineFunctionInfo *AFI = MF.getInfo<AVRMachineFunctionInfo>();
  uint64_t FrameSize = MFI->getStackSize() - AFI->getCalleeSavedFrameSize();

  // Skip the callee-saved push instructions.
  while ((MBBI != MBB.end())
         && ((MBBI->getOpcode() == AVR::PUSHRr
             || MBBI->getOpcode() == AVR::PUSHWRr)))
  {
    ++MBBI;
  }

  // Update Y with the new base value.
  BuildMI(MBB, MBBI, dl, TII.get(AVR::SPREAD), AVR::R29R28)
    .addReg(AVR::SP)
    .setMIFlag(MachineInstr::FrameSetup);

  // Mark the FramePtr as live-in in every block except the entry.
  for (MachineFunction::iterator I = llvm::next(MF.begin()), E = MF.end();
       I != E; ++I)
  {
    I->addLiveIn(AVR::R29R28);
  }

  // Reserve the necessary frame memory by doing FP -= <size>.
  if (FrameSize)
  {
    unsigned Opcode = (isUInt<6>(FrameSize)) ? AVR::SBIWRdK : AVR::SUBIWRdK;

    MachineInstr *MI = BuildMI(MBB, MBBI, dl, TII.get(Opcode), AVR::R29R28)
      .addReg(AVR::R29R28, RegState::Kill)
      .addImm(FrameSize)
      .setMIFlag(MachineInstr::FrameSetup);
    // The SREG implicit def is dead.
    MI->getOperand(3).setIsDead();

    // Write back R29R28 to SP and temporarily disable interrupts.
    BuildMI(MBB, MBBI, dl, TII.get(AVR::SPWRITE), AVR::SP)
      .addReg(AVR::R29R28)
      .setMIFlag(MachineInstr::FrameSetup);
  }
}

void AVRFrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const
{
  CallingConv::ID CallConv = MF.getFunction()->getCallingConv();
  bool isHandler = (CallConv == CallingConv::AVR_INTR
                    || CallConv == CallingConv::AVR_SIGNAL);

  // Early exit if the frame pointer is not needed in this function except for
  // signal/interrupt handlers where special code generation is needed.
  if (!hasFP(MF) && !isHandler)
  {
    return;
  }

  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  assert(MBBI->getDesc().isReturn()
         && "Can only insert epilog into returning blocks");
  DebugLoc dl = MBBI->getDebugLoc();
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const AVRMachineFunctionInfo *AFI = MF.getInfo<AVRMachineFunctionInfo>();
  uint64_t FrameSize = MFI->getStackSize() - AFI->getCalleeSavedFrameSize();
  const AVRInstrInfo &TII =
    *static_cast<const AVRInstrInfo *>(MF.getTarget().getInstrInfo());

  // Emit special epilogue code to restore R1, R0 and SREG in interrupt/signal
  // handlers at the very end of the function, just before reti.
  if (isHandler)
  {
    // Don't modify the MBBI iterator so that in the loop below we start
    // searching just before the point where these instructions are inserted.
    MachineBasicBlock::iterator MBBI2 = MBBI;
    BuildMI(MBB, MBBI2, dl, TII.get(AVR::POPRd), AVR::R0);
    BuildMI(MBB, MBBI2, dl, TII.get(AVR::OUTARr))
      .addImm(0x3f)
      .addReg(AVR::R0, RegState::Kill);
    BuildMI(MBB, MBBI2, dl, TII.get(AVR::POPWRd), AVR::R1R0);
  }

  // Early exit if there is no need to restore the frame pointer.
  if (!FrameSize)
  {
    return;
  }

  // Skip the callee-saved pop instructions.
  while (MBBI != MBB.begin())
  {
    MachineBasicBlock::iterator PI = llvm::prior(MBBI);
    int Opc = PI->getOpcode();

    if (((Opc != AVR::POPRd) && (Opc != AVR::POPWRd)) && !(PI->isTerminator()))
    {
      break;
    }

    --MBBI;
  }

  unsigned Opcode;

  // Select the optimal opcode depending on how big it is.
  if (isUInt<6>(FrameSize))
  {
    Opcode = AVR::ADIWRdK;
  }
  else
  {
    Opcode = AVR::SUBIWRdK;
    FrameSize = -FrameSize;
  }

  // Restore the frame pointer by doing FP += <size>.
  MachineInstr *MI = BuildMI(MBB, MBBI, dl, TII.get(Opcode), AVR::R29R28)
    .addReg(AVR::R29R28, RegState::Kill)
    .addImm(FrameSize);
  // The SREG implicit def is dead.
  MI->getOperand(3).setIsDead();

  // Write back R29R28 to SP and temporarily disable interrupts.
  BuildMI(MBB, MBBI, dl, TII.get(AVR::SPWRITE), AVR::SP)
    .addReg(AVR::R29R28, RegState::Kill);
}

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register. This is true if the function meets any of the following
// conditions:
//  - a register has been spilled
//  - has allocas
//  - input arguments are passed using the stack
// Notice that strictly this is not a frame pointer because it contains SP after
// frame allocation instead of having the original SP in function entry.
bool AVRFrameLowering::hasFP(const MachineFunction &MF) const
{
  const AVRMachineFunctionInfo *FuncInfo = MF.getInfo<AVRMachineFunctionInfo>();

  return (FuncInfo->getHasSpills()
          || FuncInfo->getHasAllocas()
          || FuncInfo->getHasStackArgs());
}

bool AVRFrameLowering::
spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MI,
                          const std::vector<CalleeSavedInfo> &CSI,
                          const TargetRegisterInfo *TRI) const
{
  if (CSI.empty())
  {
    return false;
  }

  unsigned CalleeFrameSize = 0;
  DebugLoc DL = MBB.findDebugLoc(MI);
  MachineFunction &MF = *MBB.getParent();
  const TargetInstrInfo &TII = *MF.getTarget().getInstrInfo();
  AVRMachineFunctionInfo *AVRFI = MF.getInfo<AVRMachineFunctionInfo>();

  for (unsigned i = CSI.size(); i != 0; --i)
  {
    unsigned Reg = CSI[i - 1].getReg();

    // Add the callee-saved register as live-in. It's killed at the spill.
    MBB.addLiveIn(Reg);
    //:FIXME: make this work with 8bit regs
    assert(TRI->getMinimalPhysRegClass(Reg)->getSize() == 2
           && "Pushing to the stack an 8 bit regiter");
    BuildMI(MBB, MI, DL, TII.get(AVR::PUSHWRr)).addReg(Reg, RegState::Kill)
      .setMIFlag(MachineInstr::FrameSetup);
    CalleeFrameSize += 2; //:TODO: when 1byte pushes work, adjust this code
  }

  AVRFI->setCalleeSavedFrameSize(CalleeFrameSize);

  return true;
}

bool AVRFrameLowering::
restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI,
                            const std::vector<CalleeSavedInfo> &CSI,
                            const TargetRegisterInfo *TRI) const
{
  if (CSI.empty())
  {
    return false;
  }

  DebugLoc DL = MBB.findDebugLoc(MI);
  const MachineFunction &MF = *MBB.getParent();
  const TargetInstrInfo &TII = *MF.getTarget().getInstrInfo();

  for (unsigned i = 0, e = CSI.size(); i != e; ++i)
  {
    unsigned Reg = CSI[i].getReg();
    //:FIXME: make this work with 8bit regs
    assert(TRI->getMinimalPhysRegClass(Reg)->getSize() == 2
           && "Popping from the stack an 8 bit regiter");
    BuildMI(MBB, MI, DL, TII.get(AVR::POPWRd), Reg);
  }

  return true;
}

void AVRFrameLowering::
processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                     RegScavenger *RS) const
{
  // Spill register Y when it is used as the frame pointer.
  if (hasFP(MF))
  {
    MF.getRegInfo().setPhysRegUsed(AVR::R29R28);
  }
}

namespace
{
  /// AVRFrameAnalyzer - Create the Frame Analyzer pass. Scan the function for
  /// allocas and used arguments that are passed through the stack.
  struct AVRFrameAnalyzer : public MachineFunctionPass
  {
    static char ID;
    AVRFrameAnalyzer() : MachineFunctionPass(ID) {}

    bool runOnMachineFunction(MachineFunction &MF)
    {
      const MachineFrameInfo *MFI = MF.getFrameInfo();
      AVRMachineFunctionInfo *FuncInfo = MF.getInfo<AVRMachineFunctionInfo>();

      // If there are no fixed frame indexes during this stage it means there
      // are allocas present in the function.
      if (MFI->getNumObjects() - MFI->getNumFixedObjects())
      {
        // Check for the type of allocas present in the function. We only care
        // about fixed size allocas so do not give false positives if only
        // variable sized allocas are present.
        for (unsigned i = 0, e = MFI->getObjectIndexEnd(); i != e; ++i)
        {
          // Variable sized objects have size 0.
          if (MFI->getObjectSize(i))
          {
            FuncInfo->setHasAllocas(true);
            break;
          }
        }
      }

      // If there are fixed frame indexes present, scan the function to see if
      // they are really being used.
      if (MFI->getNumFixedObjects() == 0)
      {
        return false;
      }

      // Ok fixed frame indexes present, now scan the function to see if they
      // are really being used, otherwise we can ignore them.
      for (MachineFunction::const_iterator BB = MF.begin(), BBE = MF.end();
           BB != BBE; ++BB)
      {
        for (MachineBasicBlock::const_iterator I = (*BB).begin(),
             E = (*BB).end(); I != E; ++I)
        {
          const MachineInstr *MI = I;
          int Opcode = MI->getOpcode();

          if ((Opcode == AVR::LDDRdPtrQ) || (Opcode == AVR::LDDWRdPtrQ)
              || (Opcode == AVR::STDPtrQRr) || (Opcode == AVR::STDWPtrQRr))
          {
            for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i)
            {
              const MachineOperand &MO = MI->getOperand(i);

              if (!MO.isFI())
              {
                continue;
              }

              int Index = MO.getIndex();
              if (MFI->isFixedObjectIndex(Index))
              {
                FuncInfo->setHasStackArgs(true);
                return false;
              }
            }
          }
        }
      }

      return false;
    }

    const char *getPassName() const
    {
      return "AVR Frame Analyzer";
    }
  };

  char AVRFrameAnalyzer::ID = 0;
} // end of anonymous namespace

/// createAVRFrameAnalyzerPass - returns an instance of the frame analyzer pass.
FunctionPass *llvm::createAVRFrameAnalyzerPass()
{
  return new AVRFrameAnalyzer();
}

namespace
{
  /// AVRDynAllocaSR - Create the Dynalloca Stack Pointer Save/Restore pass.
  /// Insert a copy of SP before allocating the dynamic stack memory and restore
  /// it in function exit to restore the original SP state. This avoids the need
  /// of reserving a register pair for a frame pointer.
  struct AVRDynAllocaSR : public MachineFunctionPass
  {
    static char ID;
    AVRDynAllocaSR() : MachineFunctionPass(ID) {}

    bool runOnMachineFunction(MachineFunction &MF)
    {
      // Early exit when there are no variable sized objects in the function.
      if (!MF.getFrameInfo()->hasVarSizedObjects())
      {
        return false;
      }

      const TargetInstrInfo *TII = MF.getTarget().getInstrInfo();
      MachineBasicBlock &EntryMBB = MF.front();
      MachineBasicBlock::iterator MBBI = EntryMBB.begin();
      DebugLoc DL = EntryMBB.findDebugLoc(MBBI);

      unsigned SPCopy =
        MF.getRegInfo().createVirtualRegister(&AVR::DREGSRegClass);

      // Create a copy of SP in function entry before any dynallocas are
      // inserted.
      BuildMI(EntryMBB, MBBI, DL, TII->get(AVR::COPY), SPCopy).addReg(AVR::SP);

      // Restore SP in all exit basic blocks.
      for (MachineFunction::iterator MFI = MF.begin(), MFE = MF.end();
           MFI != MFE; ++MFI)
      {
        // If last instruction is a return instruction, add a restore copy.
        if (!MFI->empty() && MFI->back().isReturn())
        {
          MBBI = MFI->getLastNonDebugInstr();
          DL = MBBI->getDebugLoc();
          BuildMI(*MFI, MBBI, DL, TII->get(AVR::COPY), AVR::SP)
            .addReg(SPCopy, RegState::Kill);
        }
      }

      return true;
    }

    const char *getPassName() const
    {
      return "AVR dynalloca stack pointer save/restore";
    }
  };

  char AVRDynAllocaSR::ID = 0;
} // end of anonymous namespace

/// createAVRDynAllocaSRPass - returns an instance of the dynalloca stack
/// pointer save/restore pass.
FunctionPass *llvm::createAVRDynAllocaSRPass()
{
  return new AVRDynAllocaSR();
}
