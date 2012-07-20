; RUN: llc < %s -march=avr | FileCheck %s

define i8 @read8() {
; CHECK: read8
; CHECK: in r24, 8
  %1 = load i8* inttoptr (i16 40 to i8*), align 8
  ret i8 %1
}

define i16 @read16() {
; CHECK: read16
; CHECK: in r24, 8
; CHECK: in r25, 9
  %1 = load i16* inttoptr (i16 40 to i16*), align 8
  ret i16 %1
}

define i32 @read32() {
; CHECK: read32
; CHECK: in r22, 8
; CHECK: in r23, 9
; CHECK: in r24, 10
; CHECK: in r25, 11
  %1 = load i32* inttoptr (i16 40 to i32*), align 8
  ret i32 %1
}

define i64 @read64() {
; CHECK: read64
; CHECK: in r18, 8
; CHECK: in r19, 9
; CHECK: in r20, 10
; CHECK: in r21, 11
; CHECK: in r22, 12
; CHECK: in r23, 13
; CHECK: in r24, 14
; CHECK: in r25, 15
  %1 = load i64* inttoptr (i16 40 to i64*), align 8
  ret i64 %1
}

define void @write8() {
; CHECK: write8
; CHECK: out 8
  store i8 22, i8* inttoptr (i16 40 to i8*), align 8
  ret void
}

define void @write16() {
; CHECK: write16
; CHECK: out 9
; CHECK: out 8
  store i16 1234, i16* inttoptr (i16 40 to i16*), align 8
  ret void
}

define void @write32() {
; CHECK: write32
; CHECK: out 11
; CHECK: out 10
; CHECK: out 9
; CHECK: out 8
  store i32 12345678, i32* inttoptr (i16 40 to i32*), align 8
  ret void
}

define void @write64() {
; CHECK: write64
; CHECK: out 15
; CHECK: out 14
; CHECK: out 13
; CHECK: out 12
; CHECK: out 11
; CHECK: out 10
; CHECK: out 9
; CHECK: out 8
  store i64 1234567891234567, i64* inttoptr (i16 40 to i64*), align 8
  ret void
}
