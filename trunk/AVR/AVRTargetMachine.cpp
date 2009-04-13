//===-- AVRTargetMachine.cpp - Define TargetMachine for AVR ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//                                               
//
//===----------------------------------------------------------------------===//

#include "AVR.h"
#include "AVRTargetAsmInfo.h"
#include "AVRTargetMachine.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetMachineRegistry.h"

using namespace llvm;

namespace {
  // Register the targets
  RegisterTarget<AVRTargetMachine> X("avr", "AVR 8-bit MCU");
}

AVRTargetMachine::
AVRTargetMachine(const Module &M, const std::string &FS) :
  Subtarget(M, FS), DataLayout("e-p:16:8:8-i8:8:8-i16:8:8-i32:8:8"), 
  InstrInfo(Subtarget), TLInfo(*this), 
  FrameInfo(TargetFrameInfo::StackGrowsDown, 8, 0) { }


const TargetAsmInfo *AVRTargetMachine::createTargetAsmInfo() const 
{
  return new AVRTargetAsmInfo(*this);
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

bool AVRTargetMachine::addInstSelector(PassManagerBase &PM, bool Fast) 
{
  // Install an instruction selector.
  // FIXME: PM.add(createAVRISelDag(*this));
  return false;
}

bool AVRTargetMachine::
addPrologEpilogInserter(PassManagerBase &PM, bool Fast) 
{
  return false;
}

bool AVRTargetMachine::addPreEmitPass(PassManagerBase &PM, bool Fast) 
{
  return true;
}

bool AVRTargetMachine::
addAssemblyEmitter(PassManagerBase &PM, bool Fast, bool Verbose, 
                   raw_ostream &Out) 
{
  // Output assembly language.
  PM.add(createAVRCodePrinterPass(Out, *this, Fast, Verbose));
  return false;
}

