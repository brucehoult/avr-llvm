//===-- AVRSubtarget.h - Define Subtarget for the AVR ----------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the AVR specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRSUBTARGET_H__
#define __INCLUDE_AVRSUBTARGET_H__

#include "llvm/Target/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "AVRGenSubtargetInfo.inc"

namespace llvm
{

class StringRef;

class AVRSubtarget : public AVRGenSubtargetInfo
{
  virtual void anchor();
protected:
  bool IsAsmOnly;
public:
  /// This constructor initializes the data members to match that
  /// of the specified triple.
  ///
  AVRSubtarget(const std::string &TT, const std::string &CPU,
               const std::string &FS);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options. Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef FS);
};

} // end namespace llvm

#endif //__INCLUDE_AVRSUBTARGET_H__
