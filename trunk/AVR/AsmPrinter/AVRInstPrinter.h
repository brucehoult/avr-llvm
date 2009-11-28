//===-- AVRInstPrinter.h - Convert AVR MCInst to assembly syntax ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints a AVR MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#ifndef AVRINSTPRINTER_H
#define AVRINSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm
{

  class MCOperand;

  class AVRInstPrinter : public MCInstPrinter {
  public:
    AVRInstPrinter(raw_ostream &O, const MCAsmInfo &MAI) :
      MCInstPrinter(O, MAI){
    }

    virtual void printInst(const MCInst *MI);

    // Autogenerated by tblgen.
    void printInstruction(const MCInst *MI);
    static const char *getRegisterName(unsigned RegNo);

    void printOperand(const MCInst *MI, unsigned OpNo,
                      const char *Modifier = 0);
    void printPCRelImmOperand(const MCInst *MI, unsigned OpNo);
    void printSrcMemOperand(const MCInst *MI, unsigned OpNo,
                            const char *Modifier = 0);
    void printCCOperand(const MCInst *MI, unsigned OpNo);

  };
}

#endif
