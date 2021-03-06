; RUN: opt %loadPolly %polybenchOpts %defaultOpts -polly-detect -analyze  %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func() nounwind {
bb.nph26.bb.nph26.split_crit_edge:
  %0 = load double* @beta, align 8                ; <double> [#uses=1]
  %1 = load double* @alpha, align 8               ; <double> [#uses=1]
  br label %bb5.preheader

bb4.us:                                           ; preds = %bb2.us
  store double %7, double* %scevgep30
  %2 = add nsw i64 %storemerge14.us, 1            ; <i64> [#uses=2]
  %exitcond28 = icmp eq i64 %2, 512               ; <i1> [#uses=1]
  br i1 %exitcond28, label %bb6, label %bb.nph.us

bb2.us:                                           ; preds = %bb.nph.us, %bb2.us
  %.tmp.0.us = phi double [ %10, %bb.nph.us ], [ %7, %bb2.us ] ; <double> [#uses=1]
  %storemerge23.us = phi i64 [ 0, %bb.nph.us ], [ %8, %bb2.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge23.us, i64 %storemerge14.us ; <double*> [#uses=1]
  %scevgep27 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge9, i64 %storemerge23.us ; <double*> [#uses=1]
  %3 = load double* %scevgep27, align 8           ; <double> [#uses=1]
  %4 = fmul double %3, %1                         ; <double> [#uses=1]
  %5 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %.tmp.0.us, %6                 ; <double> [#uses=2]
  %8 = add nsw i64 %storemerge23.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %8, 512                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4.us, label %bb2.us

bb.nph.us:                                        ; preds = %bb5.preheader, %bb4.us
  %storemerge14.us = phi i64 [ %2, %bb4.us ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %scevgep30 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge9, i64 %storemerge14.us ; <double*> [#uses=3]
  %9 = load double* %scevgep30, align 8           ; <double> [#uses=1]
  %10 = fmul double %9, %0                        ; <double> [#uses=2]
  store double %10, double* %scevgep30, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %11 = add nsw i64 %storemerge9, 1               ; <i64> [#uses=2]
  %12 = icmp slt i64 %11, 512                     ; <i1> [#uses=1]
  br i1 %12, label %bb5.preheader, label %return

bb5.preheader:                                    ; preds = %bb6, %bb.nph26.bb.nph26.split_crit_edge
  %storemerge9 = phi i64 [ 0, %bb.nph26.bb.nph26.split_crit_edge ], [ %11, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph.us

return:                                           ; preds = %bb6
  ret void
}

; CHECK: Valid Region for Scop: bb5.preheader => return

