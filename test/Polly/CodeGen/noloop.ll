; RUN: opt -polly-print-scop -S -analyze < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-linux-gnu"

define i64 @f([128 x i64]* nocapture %a, i64 %N, i64 %M) nounwind readonly {
entry:
  %0 = getelementptr inbounds [128 x i64]* %a, i64 %N, i64 %M ; <i64*> [#uses=1]
  %1 = load i64* %0, align 8                      ; <i64> [#uses=1]
  ret i64 %1
}

; CHECK: S0();
