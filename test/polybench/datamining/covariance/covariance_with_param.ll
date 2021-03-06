; RUN: opt %loadPolly %polybenchOpts %defaultOpts -polly-analyze-ir  -print-top-scop-only -analyze %s | FileCheck %s
; Scalar evolution cannot predict backedge taken count.
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=3]
define void @scop_func(i64 %m, i64 %n) nounwind {
entry:
  %0 = icmp slt i64 %m, 1                         ; <i1> [#uses=3]
  br i1 %0, label %bb10.preheader, label %bb.nph44

bb.nph44:                                         ; preds = %entry
  %1 = icmp slt i64 %n, 1                         ; <i1> [#uses=1]
  %2 = load double* @float_n, align 8             ; <double> [#uses=2]
  br i1 %1, label %bb3.us, label %bb.nph36

bb3.us:                                           ; preds = %bb3.us, %bb.nph44
  %indvar = phi i64 [ %tmp, %bb3.us ], [ 0, %bb.nph44 ] ; <i64> [#uses=2]
  %tmp = add i64 %indvar, 1                       ; <i64> [#uses=2]
  %scevgep = getelementptr [501 x double]* @mean, i64 0, i64 %tmp ; <double*> [#uses=1]
  %tmp45 = add i64 %indvar, 2                     ; <i64> [#uses=1]
  %3 = fdiv double 0.000000e+00, %2               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %4 = icmp sgt i64 %tmp45, %m                    ; <i1> [#uses=1]
  br i1 %4, label %bb10.preheader, label %bb3.us

bb.nph36:                                         ; preds = %bb3, %bb.nph44
  %indvar94 = phi i64 [ %tmp100, %bb3 ], [ 0, %bb.nph44 ] ; <i64> [#uses=2]
  %tmp100 = add i64 %indvar94, 1                  ; <i64> [#uses=3]
  %tmp102 = add i64 %indvar94, 2                  ; <i64> [#uses=1]
  %scevgep103 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp100 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep103, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph36
  %indvar91 = phi i64 [ 0, %bb.nph36 ], [ %tmp99, %bb1 ] ; <i64> [#uses=2]
  %5 = phi double [ 0.000000e+00, %bb.nph36 ], [ %7, %bb1 ] ; <double> [#uses=1]
  %tmp99 = add i64 %indvar91, 1                   ; <i64> [#uses=2]
  %scevgep97 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp99, i64 %tmp100 ; <double*> [#uses=1]
  %6 = load double* %scevgep97, align 8           ; <double> [#uses=1]
  %7 = fadd double %5, %6                         ; <double> [#uses=2]
  %tmp98 = add i64 %indvar91, 2                   ; <i64> [#uses=1]
  %8 = icmp sgt i64 %tmp98, %n                    ; <i1> [#uses=1]
  br i1 %8, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %9 = fdiv double %7, %2                         ; <double> [#uses=1]
  store double %9, double* %scevgep103, align 8
  %10 = icmp sgt i64 %tmp102, %m                  ; <i1> [#uses=1]
  br i1 %10, label %bb10.preheader, label %bb.nph36

bb10.preheader:                                   ; preds = %bb3, %bb3.us, %entry
  %11 = icmp slt i64 %n, 1                        ; <i1> [#uses=2]
  br i1 %11, label %bb19.preheader, label %bb.nph33

bb7:                                              ; preds = %bb8.preheader, %bb7
  %indvar77 = phi i64 [ %tmp87, %bb7 ], [ 0, %bb8.preheader ] ; <i64> [#uses=2]
  %tmp87 = add i64 %indvar77, 1                   ; <i64> [#uses=3]
  %scevgep83 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp86, i64 %tmp87 ; <double*> [#uses=2]
  %scevgep84 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp87 ; <double*> [#uses=1]
  %12 = load double* %scevgep83, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep84, align 8          ; <double> [#uses=1]
  %14 = fsub double %12, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep83, align 8
  %tmp85 = add i64 %indvar77, 2                   ; <i64> [#uses=1]
  %15 = icmp sgt i64 %tmp85, %m                   ; <i1> [#uses=1]
  br i1 %15, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %16 = icmp sgt i64 %tmp89, %n                   ; <i1> [#uses=1]
  br i1 %16, label %bb19.preheader, label %bb8.preheader

bb.nph33:                                         ; preds = %bb10.preheader
  br i1 %0, label %return, label %bb8.preheader

bb8.preheader:                                    ; preds = %bb.nph33, %bb9
  %indvar79 = phi i64 [ %tmp86, %bb9 ], [ 0, %bb.nph33 ] ; <i64> [#uses=2]
  %tmp86 = add i64 %indvar79, 1                   ; <i64> [#uses=2]
  %tmp89 = add i64 %indvar79, 2                   ; <i64> [#uses=1]
  br label %bb7

bb19.preheader:                                   ; preds = %bb9, %bb10.preheader
  br i1 %0, label %return, label %bb17.preheader

bb.nph13:                                         ; preds = %bb17.preheader
  %tmp53 = add i64 %storemerge214, 1              ; <i64> [#uses=2]
  br i1 %11, label %bb16.us, label %bb.nph13.bb.nph13.split_crit_edge

bb.nph13.bb.nph13.split_crit_edge:                ; preds = %bb.nph13
  %tmp73 = mul i64 %storemerge214, 502            ; <i64> [#uses=2]
  br label %bb.nph

bb16.us:                                          ; preds = %bb16.us, %bb.nph13
  %indvar48 = phi i64 [ %indvar.next49, %bb16.us ], [ 0, %bb.nph13 ] ; <i64> [#uses=3]
  %storemerge218 = phi i64 [ %storemerge218, %bb16.us ], [ %storemerge214, %bb.nph13 ] ; <i64> [#uses=4]
  %tmp50 = add i64 %storemerge214, %indvar48      ; <i64> [#uses=2]
  %tmp54 = add i64 %tmp53, %indvar48              ; <i64> [#uses=1]
  %tmp56 = mul i64 %storemerge218, 501            ; <i64> [#uses=1]
  %scevgep55.sum = add i64 %tmp50, %tmp56         ; <i64> [#uses=1]
  %scevgep57 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 0, i64 %scevgep55.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep57, align 8
  %scevgep52 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %tmp50, i64 %storemerge218 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep52, align 8
  %17 = icmp sgt i64 %tmp54, %m                   ; <i1> [#uses=1]
  %indvar.next49 = add i64 %indvar48, 1           ; <i64> [#uses=1]
  br i1 %17, label %bb18, label %bb16.us

bb.nph:                                           ; preds = %bb16, %bb.nph13.bb.nph13.split_crit_edge
  %indvar62 = phi i64 [ 0, %bb.nph13.bb.nph13.split_crit_edge ], [ %indvar.next63, %bb16 ] ; <i64> [#uses=5]
  %tmp69 = add i64 %storemerge214, %indvar62      ; <i64> [#uses=1]
  %tmp72 = add i64 %tmp53, %indvar62              ; <i64> [#uses=1]
  %scevgep74 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar62, i64 %tmp73 ; <double*> [#uses=1]
  %tmp75 = add i64 %tmp73, %indvar62              ; <i64> [#uses=1]
  %scevgep76 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 0, i64 %tmp75 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep76, align 8
  br label %bb14

bb14:                                             ; preds = %bb14, %bb.nph
  %indvar59 = phi i64 [ 0, %bb.nph ], [ %tmp68, %bb14 ] ; <i64> [#uses=2]
  %18 = phi double [ 0.000000e+00, %bb.nph ], [ %22, %bb14 ] ; <double> [#uses=1]
  %tmp68 = add i64 %indvar59, 1                   ; <i64> [#uses=3]
  %scevgep65 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp68, i64 %tmp69 ; <double*> [#uses=1]
  %scevgep66 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp68, i64 %storemerge214 ; <double*> [#uses=1]
  %19 = load double* %scevgep66, align 8          ; <double> [#uses=1]
  %20 = load double* %scevgep65, align 8          ; <double> [#uses=1]
  %21 = fmul double %19, %20                      ; <double> [#uses=1]
  %22 = fadd double %18, %21                      ; <double> [#uses=3]
  %tmp67 = add i64 %indvar59, 2                   ; <i64> [#uses=1]
  %23 = icmp sgt i64 %tmp67, %n                   ; <i1> [#uses=1]
  br i1 %23, label %bb16, label %bb14

bb16:                                             ; preds = %bb14
  store double %22, double* %scevgep76
  store double %22, double* %scevgep74, align 8
  %24 = icmp sgt i64 %tmp72, %m                   ; <i1> [#uses=1]
  %indvar.next63 = add i64 %indvar62, 1           ; <i64> [#uses=1]
  br i1 %24, label %bb18, label %bb.nph

bb18:                                             ; preds = %bb17.preheader, %bb16, %bb16.us
  %storemerge225 = phi i64 [ %storemerge214, %bb17.preheader ], [ %storemerge218, %bb16.us ], [ %storemerge214, %bb16 ] ; <i64> [#uses=1]
  %25 = add nsw i64 %storemerge225, 1             ; <i64> [#uses=2]
  %26 = icmp sgt i64 %25, %m                      ; <i1> [#uses=1]
  br i1 %26, label %return, label %bb17.preheader

bb17.preheader:                                   ; preds = %bb18, %bb19.preheader
  %storemerge214 = phi i64 [ %25, %bb18 ], [ 1, %bb19.preheader ] ; <i64> [#uses=9]
  %27 = icmp sgt i64 %storemerge214, %m           ; <i1> [#uses=1]
  br i1 %27, label %bb18, label %bb.nph13

return:                                           ; preds = %bb18, %bb19.preheader, %bb.nph33
  ret void
}
