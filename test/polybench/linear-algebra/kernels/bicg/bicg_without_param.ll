; RUN: opt %loadPolly %polybenchOpts %defaultOpts -polly-cloog -analyze  %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@r = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]
@p = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]
@A = common global [8000 x [8000 x double]] zeroinitializer, align 32 ; <[8000 x [8000 x double]]*> [#uses=4]
@s = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=8]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@q = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]
define void @scop_func() nounwind {
bb.nph10:
  br label %bb

bb:                                               ; preds = %bb, %bb.nph10
  %storemerge9 = phi i64 [ 0, %bb.nph10 ], [ %0, %bb ] ; <i64> [#uses=2]
  %scevgep18 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge9 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep18, align 8
  %0 = add nsw i64 %storemerge9, 1                ; <i64> [#uses=2]
  %exitcond17 = icmp eq i64 %0, 8000              ; <i1> [#uses=1]
  br i1 %exitcond17, label %bb.nph.us, label %bb

bb6.us:                                           ; preds = %bb4.us
  store double %8, double* %scevgep15
  %1 = add nsw i64 %storemerge14.us, 1            ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %1, 8000              ; <i1> [#uses=1]
  br i1 %exitcond13, label %return, label %bb.nph.us

bb4.us:                                           ; preds = %bb.nph.us, %bb4.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %8, %bb4.us ] ; <double> [#uses=1]
  %storemerge23.us = phi i64 [ 0, %bb.nph.us ], [ %9, %bb4.us ] ; <i64> [#uses=4]
  %scevgep11 = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge14.us, i64 %storemerge23.us ; <double*> [#uses=1]
  %scevgep = getelementptr [8000 x double]* @p, i64 0, i64 %storemerge23.us ; <double*> [#uses=1]
  %scevgep12 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge23.us ; <double*> [#uses=2]
  %2 = load double* %scevgep12, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep11, align 8           ; <double> [#uses=2]
  %4 = fmul double %10, %3                        ; <double> [#uses=1]
  %5 = fadd double %2, %4                         ; <double> [#uses=1]
  store double %5, double* %scevgep12, align 8
  %6 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %7 = fmul double %3, %6                         ; <double> [#uses=1]
  %8 = fadd double %.tmp.0.us, %7                 ; <double> [#uses=2]
  %9 = add nsw i64 %storemerge23.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %9, 8000                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.us, label %bb4.us

bb.nph.us:                                        ; preds = %bb6.us, %bb
  %storemerge14.us = phi i64 [ %1, %bb6.us ], [ 0, %bb ] ; <i64> [#uses=4]
  %scevgep15 = getelementptr [8000 x double]* @q, i64 0, i64 %storemerge14.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep15, align 8
  %scevgep16 = getelementptr [8000 x double]* @r, i64 0, i64 %storemerge14.us ; <double*> [#uses=1]
  %10 = load double* %scevgep16, align 8          ; <double> [#uses=1]
  br label %bb4.us

return:                                           ; preds = %bb6.us
  ret void
}

; CHECK:      Printing analysis 'Execute Cloog code generation' for region: 'bb.nph.us => return' in function 'scop_func':
; CHECK-NEXT: scop_func():

