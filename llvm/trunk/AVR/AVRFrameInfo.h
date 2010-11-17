//===-- AVRFrameInfo.h - Define TargetFrameInfo for AVR --*- C++ -*--===//
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

#ifndef AVR_FRAMEINFO_H
#define AVR_FRAMEINFO_H

#include "AVR.h"
#include "AVRSubtarget.h"
#include "llvm/Target/TargetFrameInfo.h"

namespace llvm {
  class AVRSubtarget;

class AVRFrameInfo : public TargetFrameInfo {
protected:
  const AVRSubtarget &STI;

public:
  explicit AVRFrameInfo(const AVRSubtarget &sti)
    : TargetFrameInfo(TargetFrameInfo::StackGrowsDown, 8, 0), STI(sti) {
  }

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologue(MachineFunction &MF) const;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const;
};

} // End llvm namespace

#endif

