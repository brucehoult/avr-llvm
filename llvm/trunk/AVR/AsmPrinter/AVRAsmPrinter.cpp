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
#include "llvm/LLVMContext.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/CodeGen/AsmPrinter.h"
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
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace
{
  class AVRAsmPrinter : public AsmPrinter
  {
    public:
      AVRAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
        : AsmPrinter(TM, Streamer) {}


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

      void printOperand(const MachineInstr *MI, int OpNum,
                      raw_ostream &O, const char* Modifier = 0);
      void EmitInstruction(const MachineInstr *MI);
      //void EmitGlobalVariable(const GlobalVariable* GVar);

  };
} // end of anonymous namespace

#if 0
// renamed/changed to EmitGlobalVariable most targets don't override it now though
// TODO: see if we can avoid adding this function.
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

  MCSymbol *GVarSym = GetGlobalValueSymbol(GVar);
  Constant *C = GVar->getInitializer();
  unsigned Size = TD->getTypeAllocSize(C->getType());
  unsigned Align = TD->getPreferredAlignmentLog(GVar);

  EmitVisibility(GVarSym, GVar->getVisibility());

  if (!GVar->hasCommonLinkage())
  {
    O << "\t.type\t" << *GVarSym << ",@object\n";
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
      O << "\t.local\t" << *GVarSym << '\n';
    }
    if (MAI->hasLCOMMDirective())
    {
      O << "\t.lcomm\t" << *GVarSym << ',' << Size;
    }
    //if (MAI->getCOMMDirectiveTakesAlignment())
    //{
    //  O << ',' << (MAI->getAlignmentIsInBytes() ? (1 << Align) : Align);
    //}
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
      O << "\t.weak\t" << *GVarSym << '\n';
      break;
    case GlobalValue::AppendingLinkage:
      // FIXME: appending linkage variables should go into a section of
      // their name or something.  For now, just emit them as external.
    case GlobalValue::ExternalLinkage:
      // If external or appending, declare as a global symbol
      O << ".global " << *GVarSym << '\n';
      // FALL THROUGH
    case GlobalValue::PrivateLinkage:
    case GlobalValue::LinkerPrivateLinkage:
      break;
    case GlobalValue::InternalLinkage:
      O << "\t.internal " << *GVarSym << '\n';
      break;
    default:
      llvm_unreachable("Unknown linkage type!");
  }
  
  /// _HACK_ (wrong place, wrong way) to match avr-gcc output
  /// TODO: Need to find were the .data directive is in LLVM <---inside EmitGlobalConstant
  O << "\t.data\n";
  if (MAI->hasDotTypeDotSizeDirective())
  {
    O << "\t.size\t" << *GVarSym << ", " << Size << '\n';
  }
  // Use 16-bit alignment by default to simplify bunch of stuff
  EmitAlignment(Align, GVar);
  O << *GVarSym << ":";
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
#endif

void AVRAsmPrinter::printOperand(const MachineInstr *MI, int OpNum,
                                    raw_ostream &O, const char *Modifier) {
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
      O << *MO.getMBB()->getSymbol();
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




//===----------------------------------------------------------------------===//
void AVRAsmPrinter::EmitInstruction(const MachineInstr *MI)
{

  AVRMCInstLower MCInstLowering(OutContext, *Mang, *this);

  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);
  OutStreamer.EmitInstruction(TmpInst);
}

static MCInstPrinter *createAVRMCInstPrinter(const Target &T,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI) 
{
  if (SyntaxVariant == 0)
    return new AVRInstPrinter(MAI);
  return 0;
}

// Force static initialization.
extern "C" void LLVMInitializeAVRAsmPrinter()
{
  RegisterAsmPrinter<AVRAsmPrinter> X(TheAVRTarget);
  TargetRegistry::RegisterMCInstPrinter(TheAVRTarget,
                                        createAVRMCInstPrinter);
}
