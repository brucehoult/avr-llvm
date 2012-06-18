; RUN: llc < %s -march=avr | FileCheck %s

;TODO: test returning byval structs

define void @return_void() {
; CHECK: return_void:{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: #{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: ret
    ret void
}

define i8 @return8_imm() {
; CHECK: return8_imm:
; CHECK: ldi r24, 5
    ret i8 5
}

define i8 @return8_arg(i8 %x) {
; CHECK: return8_arg:{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: #{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: ret
    ret i8 %x
}

define i8 @return8_arg2(i8 %x, i8 %y, i8 %z) {
; CHECK: return8_arg2:
; CHECK: mov r24, r20
    ret i8 %z
}

define i16 @return16_imm() {
; CHECK: return16_imm:
; CHECK: ldi r24, 57
; CHECK: ldi r25, 48
    ret i16 12345
}

define i16 @return16_arg(i16 %x) {
; CHECK: return16_arg:{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: #{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: ret
    ret i16 %x
}

define i16 @return16_arg2(i16 %x, i16 %y, i16 %z) {
; CHECK: return16_arg2:
; CHECK: movw r25:r24, r21:r20
    ret i16 %z
}

define i32 @return32_imm() {
; CHECK: return32_imm:
; CHECK: ldi r22, 21
; CHECK: ldi r23, 205
; CHECK: ldi r24, 91
; CHECK: ldi r25, 7
    ret i32 123456789
}

define i32 @return32_arg(i32 %x) {
; CHECK: return32_arg:{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: #{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: ret
    ret i32 %x
}

define i32 @return32_arg2(i32 %x, i32 %y, i32 %z) {
; CHECK: return32_arg2:
; CHECK: push r14
; CHECK: push r15
; CHECK: push r16
; CHECK: push r17
; CHECK: movw r23:r22, r15:r14
; CHECK: movw r25:r24, r17:r16
; CHECK: pop r17
; CHECK: pop r16
; CHECK: pop r15
; CHECK: pop r14
    ret i32 %z
}

define i64 @return64_imm() {
; CHECK: return64_imm:
; CHECK: ldi r18, 204
; CHECK: ldi r19, 204
; CHECK: ldi r20, 104
; CHECK: ldi r21, 37
; CHECK: ldi r22, 25
; CHECK: ldi r23, 22
; CHECK: ldi r24, 236
; CHECK: ldi r25, 190
    ret i64 13757395258967641292
}

define i64 @return64_arg(i64 %x) {
; CHECK: return64_arg:{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: #{{[a-zA-Z0-9 #@]*}}
; CHECK-NEXT: ret
    ret i64 %x
}

define i64 @return64_arg2(i64 %x, i64 %y, i64 %z) {
; CHECK: return64_arg2:
; CHECK: push r28
; CHECK: push r29
; CHECK: ldd r18, Y+5
; CHECK: ldd r19, Y+6
; CHECK: ldd r20, Y+7
; CHECK: ldd r21, Y+8
; CHECK: ldd r22, Y+9
; CHECK: ldd r23, Y+10
; CHECK: ldd r24, Y+11
; CHECK: ldd r25, Y+12
; CHECK: pop r29
; CHECK: pop r28
    ret i64 %z
}

define i32 @return64_trunc(i32 %a, i32 %b, i32 %c, i64 %d) {
; CHECK: return64_trunc:
; CHECK: push r28
; CHECK: push r29
; CHECK: ldd r22, Y+5
; CHECK: ldd r23, Y+6
; CHECK: ldd r24, Y+7
; CHECK: ldd r25, Y+8
; CHECK: pop r29
; CHECK: pop r28
  %result = trunc i64 %d to i32
  ret i32 %result
}

