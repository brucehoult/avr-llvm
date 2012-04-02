//===-- AVRInstPrinter.cpp - Convert AVR MCInst to assembly syntax --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an AVR MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"

#include "AVRInstPrinter.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include <cstring>

using namespace llvm;

// Include the auto-generated portion of the assembly writer.
#include "AVRGenAsmWriter.inc"

//:FIXME: this should be done somewhere else
// check out the new feature about alternative reg names
static const char *getPtrRegName(const char *reg)
{
  if (strcmp(reg, "r31:r30") == 0)
  {
    return "Z";
  }
  else if (strcmp(reg, "r29:r28") == 0)
  {
    return "Y";
  }
  else if (strcmp(reg, "r27:r26") == 0)
  {
    return "X";
  }
  else if (strcmp(reg, "SP") == 0)
  {
    //:FIXME: SP is used for pseudo ld/st that get expanded into in/out
    return "SP";
  }
  else
  {
    llvm_unreachable("Invalid register name");
  }

  return 0;
}

void AVRInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                               StringRef Annot)
{
  unsigned Opcode = MI->getOpcode();

  // First handle load and store instructions with postinc or predec
  // of the form "ld reg, X+"
  switch (Opcode)
  {
  case AVR::LDRdPtr:
  case AVR::LDRdPtrPi:
  case AVR::LDRdPtrPd:
    O << "\tld\t";
    printOperand(MI, 0, O);
    O << ", ";
    if (Opcode == AVR::LDRdPtrPd) O << '-';
    printOperand(MI, 1, O, "ptr");
    if (Opcode == AVR::LDRdPtrPi) O << '+';
    return;
  case AVR::STPtrRr:
    O << "\tst\t";
    printOperand(MI, 0, O, "ptr");
    O << ", ";
    printOperand(MI, 1, O);
    return;
  case AVR::STPtrPiRr:
  case AVR::STPtrPdRr:
    O << "\tst\t";
    if (Opcode == AVR::STPtrPdRr) O << '-';
    printOperand(MI, 1, O, "ptr");
    if (Opcode == AVR::STPtrPiRr) O << '+';
    O << ", ";
    printOperand(MI, 2, O);
    return;
  }

  printInstruction(MI, O);
  printAnnotation(O, Annot);
}

void AVRInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                  raw_ostream &O, const char *Modifier)
{
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg())
  {
    // Print ptr registers as X, Y and Z
    if (Modifier && strcmp(Modifier, "ptr") == 0)
    {
      O << getPtrRegName(getRegisterName(Op.getReg()));
    }
    else
    {
      O << getRegisterName(Op.getReg());
    }
  }
  else if (Op.isImm())
  {
    O << Op.getImm();
  }
  else
  {
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    O << *Op.getExpr();
  }
}

void AVRInstPrinter::printMemriOperand(const MCInst *MI, unsigned OpNo,
                                       raw_ostream &O)
{
  // The memri operand is of the form "Y+<offs>"
  printOperand(MI, OpNo, O, "ptr");
  O << '+';
  printOperand(MI, OpNo + 1, O);
}

/// print_pcrel_imm - This is used to print an immediate value that ends up
/// being encoded as a pc-relative value.
void AVRInstPrinter::print_pcrel_imm(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O)
{
  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isImm())
  {
    O << Op.getImm();
  }
  else
  {
    assert(Op.isExpr() && "unknown pcrel immediate operand");
    O << *Op.getExpr();
  }
}
