//=======- AVRFrameInfo.cpp - AVR Frame Information ------*- C++ -*-====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the AVR implementation of TargetFrameInfo class.
//
//===----------------------------------------------------------------------===//

#include "AVRFrameInfo.h"
//#include "AVRInstrInfo.h"
//#include "AVRMachineFunctionInfo.h"
//#include "llvm/Function.h"
//#include "llvm/CodeGen/MachineFrameInfo.h"
//#include "llvm/CodeGen/MachineFunction.h"
//#include "llvm/CodeGen/MachineInstrBuilder.h"
//#include "llvm/CodeGen/MachineModuleInfo.h"
//#include "llvm/CodeGen/MachineRegisterInfo.h"
//#include "llvm/Target/TargetData.h"
//#include "llvm/Target/TargetOptions.h"
//#include "llvm/Support/CommandLine.h"

using namespace llvm;

void AVRFrameInfo::emitPrologue(MachineFunction &MF) const {}

void AVRFrameInfo::emitEpilogue(MachineFunction &MF,
                                   MachineBasicBlock &MBB) const {}

