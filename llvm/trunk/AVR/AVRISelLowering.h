//===-- AVRISelLowering.h - AVR DAG Lowering Interface ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that AVR uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRISELLOWERING_H__
#define __INCLUDE_AVRISELLOWERING_H__

#include "llvm/Target/TargetLowering.h"

namespace llvm
{

namespace AVRISD
{
  /// AVR Specific DAG Nodes
  enum NodeType
  {
    /// Start the numbering where the builtin ops leave off.
    FIRST_NUMBER = ISD::BUILTIN_OP_END,
    /// Return from subroutine.
    RET_FLAG,
    /// CALL - These operations represent an abstract call
    /// instruction, which includes a bunch of information.
    CALL,
    /// Wrapper - A wrapper node for TargetConstantPool,
    /// TargetExternalSymbol, and TargetGlobalAddress.
    Wrapper,
    /// Bit shifting and rotation.
    LSL,
    LSR,
    ASR,
    ROR,
    ROL,
    /// Load SP to a physical reg from I/O space, it is a
    /// normal load but with in and out glue operands.
    SPLOAD,
    /// AVR conditional branches. Operand 0 is the chain operand, operand 1
    /// is the block to branch if condition is true, operand 2 is the
    /// condition code, and operand 3 is the flag operand produced by a CMP
    /// or TEST instruction.
    BRCOND,
    /// CMP - Compare instruction.
    CMP,
    /// CMPC - Compare with carry instruction.
    CMPC,
    /// TST - Test for zero or minus instruction.
    TST,
    /// SELECT_CC - Operand 0 and operand 1 are selection variable, operand 2
    /// is condition code and operand 3 is flag operand.
    SELECT_CC
  };
}

class AVRTargetMachine;

class AVRTargetLowering : public TargetLowering
{
public:
  explicit AVRTargetLowering(AVRTargetMachine &TM);
public: // TargetLowering
  MVT getShiftAmountTy(EVT LHSTy) const { return MVT::i8; }
  const char *getTargetNodeName(unsigned Opcode) const;

  /// LowerOperation - Provide custom lowering hooks for some operations.
  ///
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;

  /// ReplaceNodeResults - Replace the results of node with an illegal result
  /// type with new values built out of custom code.
  ///
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const;

  /// isLegalAddressingMode - Return true if the addressing mode represented
  /// by AM is legal for this target, for a load/store of the specified type.
  ///
  bool isLegalAddressingMode(const AddrMode &AM, Type *Ty) const;

  /// getPreIndexedAddressParts - returns true by value, base pointer and
  /// offset pointer and addressing mode by reference if the node's address
  /// can be legally represented as pre-indexed load / store address.
  bool getPreIndexedAddressParts(SDNode *N, SDValue &Base, SDValue &Offset,
                                 ISD::MemIndexedMode &AM,
                                 SelectionDAG &DAG) const;

  /// getPostIndexedAddressParts - returns true by value, base pointer and
  /// offset pointer and addressing mode by reference if this node can be
  /// combined with a load / store to form a post-indexed load / store.
  bool getPostIndexedAddressParts(SDNode *N, SDNode *Op, SDValue &Base,
                                  SDValue &Offset, ISD::MemIndexedMode &AM,
                                  SelectionDAG &DAG) const;

  /// isOffsetFoldingLegal - Return true if folding a constant offset
  /// with the given GlobalAddress is legal.  It is frequently not legal in
  /// PIC relocation models.
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const;

  MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr *MI,
                                                 MachineBasicBlock *MBB) const;
private:
  SDValue getAVRCmp(SDValue LHS, SDValue RHS, ISD::CondCode CC, SDValue &AVRcc,
                    SelectionDAG &DAG, DebugLoc dl) const;
  SDValue LowerShifts(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, DebugLoc dl,
                      SelectionDAG &DAG) const;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               DebugLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerCall(SDValue Chain, SDValue Callee, CallingConv::ID CallConv,
                    bool isVarArg, bool &isTailCall,
                    const SmallVectorImpl<ISD::OutputArg> &Outs,
                    const SmallVectorImpl<SDValue> &OutVals,
                    const SmallVectorImpl<ISD::InputArg> &Ins, DebugLoc dl,
                    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool isVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          DebugLoc dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;
private:
  const TargetData *TD;
  const AVRTargetMachine &TM;
};

} // end namespace llvm

#endif //__INCLUDE_AVRISELLOWERING_H__
