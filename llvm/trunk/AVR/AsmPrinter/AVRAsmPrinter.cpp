//===-- AVRAsmPrinter.cpp - AVR LLVM assembly writer ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the AVR assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "AVR.h"
#include "AVRInstrInfo.h"
#include "AVRInstPrinter.h"
#include "AVRMCAsmInfo.h"
#include "AVRMCInstLower.h"
#include "AVRTargetMachine.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/DwarfWriter.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Mangler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

STATISTIC(EmittedInsts, "Number of machine instrs printed");

namespace
{
  class AVRAsmPrinter : public AsmPrinter
  {
    public:
      AVRAsmPrinter(formatted_raw_ostream &O, TargetMachine &TM,
                       const MCAsmInfo *MAI, bool V)
        : AsmPrinter(O, TM, MAI, V){}

      virtual const char *getPassName() const
      {
        return "AVR Assembly Printer";
      }
      virtual void EmitStartOfAsmFile(Module &M)
      {
        M.setModuleInlineAsm ("__SREG__ = 0x3f\n"
                              "__SP_H__ = 0x3e\n"
                              "__SP_L__ = 0x3d\n"
                              "__CCP__  = 0x34\n"
                              "__tmp_reg__ = 0\n"
                              "__zero_reg__ = 1");
      }
      void printMCInst(const MCInst *MI)
      {
        AVRInstPrinter(O, *MAI).printInstruction(MI);
      }
      void printOperand(const MachineInstr *MI, int OpNum);
      
      void printPCRelImmOperand(const MachineInstr *MI, int OpNum)
      {
        printOperand(MI, OpNum);
      }
      void printSrcMemOperand(const MachineInstr *MI, int OpNum);
      void printCCOperand(const MachineInstr *MI, int OpNum);
      void printMachineInstruction(const MachineInstr * MI);
      bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                           unsigned AsmVariant,
                           const char *ExtraCode);
      bool PrintAsmMemoryOperand(const MachineInstr *MI,
                                 unsigned OpNo, unsigned AsmVariant,
                                 const char *ExtraCode);
      void printInstructionThroughMCStreamer(const MachineInstr *MI);

      void PrintGlobalVariable(const GlobalVariable* GVar);
      void emitFunctionHeader(const MachineFunction &MF);
      bool runOnMachineFunction(MachineFunction &F);

      void getAnalysisUsage(AnalysisUsage &AU) const
      {
        AsmPrinter::getAnalysisUsage(AU);
        AU.setPreservesAll();
      }
  };
} // end of anonymous namespace

void AVRAsmPrinter::PrintGlobalVariable(const GlobalVariable* GVar)
{
  if (!GVar->hasInitializer())
  {
    return;   // External global require no code
  }
  // Check to see if this is a special global used by LLVM, if so, emit it.
  if (EmitSpecialLLVMGlobal(GVar))
    return;

  const TargetData *TD = TM.getTargetData();

  std::string name = Mang->getMangledName(GVar);
  Constant *C = GVar->getInitializer();
  unsigned Size = TD->getTypeAllocSize(C->getType());
  unsigned Align = TD->getPreferredAlignmentLog(GVar);

  printVisibility(name, GVar->getVisibility());

  if (!GVar->hasCommonLinkage())
  {
    O << "\t.type\t" << name << ",@object\n";
  }
///avr-gcc doesn't produce section info for globals
// OutStreamer.SwitchSection(getObjFileLowering().SectionForGlobal(GVar, Mang,
//                                                             TM));

  if (C->isNullValue() && !GVar->hasSection() &&
      !GVar->isThreadLocal() &&
      (GVar->hasLocalLinkage() || GVar->isWeakForLinker())) 
  {
    if (Size == 0) Size = 1;   // .comm Foo, 0 is undefined, avoid it.

    if (GVar->hasLocalLinkage())
    {
      O << "\t.local\t" << name << '\n';
    }
    O << MAI->getCOMMDirective()  << name << ',' << Size;
    if (MAI->getCOMMDirectiveTakesAlignment())
    {
      O << ',' << (MAI->getAlignmentIsInBytes() ? (1 << Align) : Align);
    }
    if (VerboseAsm)
    {
      O.PadToColumn(MAI->getCommentColumn());
      O << MAI->getCommentString() << ' ';
      WriteAsOperand(O, GVar, /*PrintType=*/false, GVar->getParent());
    }
    O << '\n';
    /// _HACK_(wrong place, wrong way) add clear bss to match avr-gcc
    O << ".global __do_clear_bss\n";
    return;
  }

  switch (GVar->getLinkage()) 
  {
    case GlobalValue::CommonLinkage:
    case GlobalValue::LinkOnceAnyLinkage:
    case GlobalValue::LinkOnceODRLinkage:
    case GlobalValue::WeakAnyLinkage:
    case GlobalValue::WeakODRLinkage:
      O << "\t.weak\t" << name << '\n';
      break;
    case GlobalValue::AppendingLinkage:
      // FIXME: appending linkage variables should go into a section of
      // their name or something.  For now, just emit them as external.
    case GlobalValue::ExternalLinkage:
      // If external or appending, declare as a global symbol
      O << ".global " << name << '\n';
      // FALL THROUGH
    case GlobalValue::PrivateLinkage:
    case GlobalValue::LinkerPrivateLinkage:
      break;
    case GlobalValue::InternalLinkage:
      O << "\t.internal " << name << '\n';
      break;
    default:
      assert(0 && "Unknown linkage type!");
  }
  
  /// _HACK_ (wrong place, wrong way) to match avr-gcc output
  /// TODO: Need to find were the .data directive is in LLVM
  O << "\t.data\n";
  if (MAI->hasDotTypeDotSizeDirective())
  {
    O << "\t.size\t" << name << ", " << Size << '\n';
  }
  // Use 16-bit alignment by default to simplify bunch of stuff
  EmitAlignment(Align, GVar);
  O << name << ":";
  if (VerboseAsm) 
  {
    O.PadToColumn(MAI->getCommentColumn());
    O << MAI->getCommentString() << ' ';
    WriteAsOperand(O, GVar, /*PrintType=*/false, GVar->getParent());
  }
  O << '\n';

  EmitGlobalConstant(C);
  /// _HACK_(wrong place, wrong way) add clear bss to match avr-gcc
  O << ".global __do_clear_bss\n";
}

void AVRAsmPrinter::emitFunctionHeader(const MachineFunction &MF) 
{
  const Function *F = MF.getFunction();

  OutStreamer.SwitchSection(getObjFileLowering().SectionForGlobal(F, Mang, TM));

  switch (F->getLinkage()) 
  {
    default: llvm_unreachable("Unknown linkage type!");
    case Function::InternalLinkage:  // Symbols default to internal.
    case Function::PrivateLinkage:
    case Function::LinkerPrivateLinkage:
      break;
    case Function::ExternalLinkage:
      O << "\t.global\t" << CurrentFnName << '\n';
      break;
    case Function::LinkOnceAnyLinkage:
    case Function::LinkOnceODRLinkage:
    case Function::WeakAnyLinkage:
    case Function::WeakODRLinkage:
      O << "\t.weak\t" << CurrentFnName << '\n';
      break;
  }

  printVisibility(CurrentFnName, F->getVisibility());

  O << "\t.type\t" << CurrentFnName << ",@function\n"
    << CurrentFnName << ":\n";
}

bool AVRAsmPrinter::runOnMachineFunction(MachineFunction &MF)
{
  SetupMachineFunction(MF);
  O << "\n\n";

  // Print the 'header' of function
  emitFunctionHeader(MF);

  // Print out code for the function.
  for (MachineFunction::const_iterator I = MF.begin(), E = MF.end();
        I != E; ++I)
  {
    // Print a label for the basic block.
    EmitBasicBlockStart(I);

    for (MachineBasicBlock::const_iterator II = I->begin(), E = I->end();
         II != E; ++II)
      // Print the assembly for the instruction.
      printMachineInstruction(II);
  }

  if (MAI->hasDotTypeDotSizeDirective())
    O << "\t.size\t" << CurrentFnName << ", .-" << CurrentFnName << '\n';

  // We didn't modify anything
  return false;
}

void AVRAsmPrinter::printMachineInstruction(const MachineInstr *MI)
{
  ++EmittedInsts;

  processDebugLoc(MI, true);

  printInstructionThroughMCStreamer(MI);

  if (VerboseAsm)
    EmitComments(*MI);
  O << '\n';

  processDebugLoc(MI, false);
}

void AVRAsmPrinter::printOperand(const MachineInstr *MI, int OpNum)
{
  const MachineOperand &MO = MI->getOperand(OpNum);
  switch (MO.getType()) 
  {
    case MachineOperand::MO_Register:
	     O << AVRInstPrinter::getRegisterName(MO.getReg());
	     return;
    case MachineOperand::MO_Immediate:
      O << MO.getImm();
      return;
    case MachineOperand::MO_MachineBasicBlock:
      GetMBBSymbol(MO.getMBB()->getNumber())->print(O, MAI);
      return;
    case MachineOperand::MO_GlobalAddress:
//- MSP430 asm specific. keeping as reference for now      
#if 0
    {
      bool isMemOp  = Modifier && !strcmp(Modifier, "mem");
      std::string Name = Mang->getMangledName(MO.getGlobal());
      uint64_t Offset = MO.getOffset();

      O << (isMemOp ? '&' : '#');
      if (Offset)
      {
        O << '(' << Offset << '+';
	     }
      O << Name;
      if (Offset)
	     {
        O << ')';
	     }
      return;
    }
#endif
    case MachineOperand::MO_ExternalSymbol:
#if 0
    {
      bool isMemOp  = Modifier && !strcmp(Modifier, "mem");
      std::string Name(MAI->getGlobalPrefix());
      Name += MO.getSymbolName();

      O << (isMemOp ? '&' : '#') << Name;

      return;
    }
#endif
    default:
      llvm_unreachable("Not implemented yet!");
  }
}

void AVRAsmPrinter::printSrcMemOperand(const MachineInstr *MI, int OpNum)
{
//- MSP430 asm specific. keeping as reference for now      
#if 0
  const MachineOperand &Base = MI->getOperand(OpNum);
  const MachineOperand &Disp = MI->getOperand(OpNum+1);

  // Print displacement first
  if (!Disp.isImm())
  {
    printOperand(MI, OpNum+1, "mem");
  } 
  else 
  {
    if (!Base.getReg())
    {
      O << '&';
    }
    printOperand(MI, OpNum+1, "nohash");
  }

  // Print register base field
  if (Base.getReg()) 
  {
    O << '(';
    printOperand(MI, OpNum);
    O << ')';
  }
#endif 
  llvm_unreachable("Not implemented yet!");
}

/// Condition Code Operand (for conditional statements)
void AVRAsmPrinter::printCCOperand(const MachineInstr *MI, int OpNum) 
{
  unsigned CC = MI->getOperand(OpNum).getImm();
#if 0 //-MSP430CC defined in MSP430.h
  switch (CC) 
  {
    default:
      llvm_unreachable("Unsupported CC code");
      break;
    case MSP430CC::COND_E:
      O << "eq";
      break;
    case MSP430CC::COND_NE:
      O << "ne";
      break;
    case MSP430CC::COND_HS:
      O << "hs";
      break;
    case MSP430CC::COND_LO:
      O << "lo";
       break;
    case MSP430CC::COND_GE:
      O << "ge";
       break;
    case MSP430CC::COND_L:
      O << 'l';
      break;
  }
#endif
}

/// PrintAsmOperand - Print out an operand for an inline asm expression.
///
bool AVRAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                     unsigned AsmVariant,const char *ExtraCode)
{
  // Does this asm operand have a single letter operand modifier?
  if (ExtraCode && ExtraCode[0])
  {
    return true; // Unknown modifier.
  }
  printOperand(MI, OpNo);
  return false;
}

bool AVRAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI, 
                                          unsigned OpNo, unsigned AsmVariant, 
                                          const char *ExtraCode)
{
                                             
  if (ExtraCode && ExtraCode[0])
  {
    return true; // Unknown modifier.
  }
  printSrcMemOperand(MI, OpNo);
  return false;
}

//===----------------------------------------------------------------------===//
void AVRAsmPrinter::printInstructionThroughMCStreamer(const MachineInstr *MI)
{

  AVRMCInstLower MCInstLowering(OutContext, *Mang, *this);

  switch (MI->getOpcode())
  {
    case TargetInstrInfo::DBG_LABEL:
    case TargetInstrInfo::EH_LABEL:
    case TargetInstrInfo::GC_LABEL:
      printLabel(MI);
      return;
    case TargetInstrInfo::KILL:
      printKill(MI);
      return;
    case TargetInstrInfo::INLINEASM:
      printInlineAsm(MI);
      return;
    case TargetInstrInfo::IMPLICIT_DEF:
      printImplicitDef(MI);
      return;
    default: break;
  }

  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);

  printMCInst(&TmpInst);
}

static MCInstPrinter *createAVRMCInstPrinter(const Target &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              raw_ostream &O) 
{
  if (SyntaxVariant == 0)
    return new AVRInstPrinter(O, MAI);
  return 0;
}

// Force static initialization.
extern "C" void LLVMInitializeAVRAsmPrinter()
{
  RegisterAsmPrinter<AVRAsmPrinter> X(TheAVRTarget);
  TargetRegistry::RegisterMCInstPrinter(TheAVRTarget,
                                        createAVRMCInstPrinter);
}
