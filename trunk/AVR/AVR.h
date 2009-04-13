//===-- AVR.h - Top-level interface for AVR representation ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in 
// the LLVM AVR back-end.
//
//===----------------------------------------------------------------------===//

#ifndef TARGET_AVR_H
#define TARGET_AVR_H

#include <iosfwd>

namespace llvm {
  class AVRTargetMachine;
  class FunctionPassManager;
  class FunctionPass;
  class MachineCodeEmitter;
  class raw_ostream;

  FunctionPass *createAVRISelDag(AVRTargetMachine &TM);
  FunctionPass *createAVRCodePrinterPass(raw_ostream &OS, 
                                         AVRTargetMachine &TM,
					bool fast, bool verbose);
} // end namespace llvm;

// Defines symbolic names for AVR registers.  This defines a mapping from
// register name to register number.
#include "AVRGenRegisterNames.inc"

// Defines symbolic names for the AVR instructions.
#include "AVRGenInstrNames.inc"

#endif
