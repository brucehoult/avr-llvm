//===-- AVRTargetInfo.cpp - AVR Target Implementation -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "AVR.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetRegistry.h"

using namespace llvm;

Target llvm::TheAVRTarget;

extern "C" void LLVMInitializeAVRTargetInfo() 
{ 
  RegisterTarget<Triple::avr, /*HasJIT=*/false>
    X(TheAVRTarget, "avr", "AVR [experimental]");
}
