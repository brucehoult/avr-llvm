//===-- AVRMCAsmInfo.h - AVR asm properties --------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the AVRMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRMCASMINFO_H__
#define __INCLUDE_AVRMCASMINFO_H__

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/ADT/StringRef.h"

namespace llvm
{

class Target;

class AVRMCAsmInfo : public MCAsmInfo
{
  virtual void anchor();
public:
  explicit AVRMCAsmInfo(const Target &T, StringRef TT);
};

} // end namespace llvm

#endif //__INCLUDE_AVRMCASMINFO_H__
