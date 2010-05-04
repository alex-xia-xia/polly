; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@F = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@G = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph61.bb.nph61.split_crit_edge:
  br label %bb.nph56

bb.nph56:                                         ; preds = %bb3, %bb.nph61.bb.nph61.split_crit_edge
  %indvar109 = phi i64 [ 0, %bb.nph61.bb.nph61.split_crit_edge ], [ %indvar.next110, %bb3 ] ; <i64> [#uses=3]
  %storemerge57 = trunc i64 %indvar109 to i32     ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge57 to double         ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph56
  %indvar106 = phi i64 [ 0, %bb.nph56 ], [ %indvar.next107, %bb1 ] ; <i64> [#uses=3]
  %scevgep111 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar109, i64 %indvar106 ; <double*> [#uses=1]
  %storemerge1355 = trunc i64 %indvar106 to i32   ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge1355 to double       ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep111, align 8
  %indvar.next107 = add i64 %indvar106, 1         ; <i64> [#uses=2]
  %exitcond108 = icmp eq i64 %indvar.next107, 512 ; <i1> [#uses=1]
  br i1 %exitcond108, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next110 = add i64 %indvar109, 1         ; <i64> [#uses=2]
  %exitcond112 = icmp eq i64 %indvar.next110, 512 ; <i1> [#uses=1]
  br i1 %exitcond112, label %bb.nph49, label %bb.nph56

bb.nph49:                                         ; preds = %bb9, %bb3
  %indvar101 = phi i64 [ %indvar.next102, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %storemerge150 = trunc i64 %indvar101 to i32    ; <i32> [#uses=1]
  %4 = sitofp i32 %storemerge150 to double        ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph49
  %indvar98 = phi i64 [ 0, %bb.nph49 ], [ %indvar.next99, %bb7 ] ; <i64> [#uses=3]
  %scevgep103 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar101, i64 %indvar98 ; <double*> [#uses=1]
  %storemerge1248 = trunc i64 %indvar98 to i32    ; <i32> [#uses=1]
  %5 = sitofp i32 %storemerge1248 to double       ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep103, align 8
  %indvar.next99 = add i64 %indvar98, 1           ; <i64> [#uses=2]
  %exitcond100 = icmp eq i64 %indvar.next99, 512  ; <i1> [#uses=1]
  br i1 %exitcond100, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %indvar.next102 = add i64 %indvar101, 1         ; <i64> [#uses=2]
  %exitcond104 = icmp eq i64 %indvar.next102, 512 ; <i1> [#uses=1]
  br i1 %exitcond104, label %bb.nph42, label %bb.nph49

bb.nph42:                                         ; preds = %bb15, %bb9
  %indvar93 = phi i64 [ %indvar.next94, %bb15 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %storemerge243 = trunc i64 %indvar93 to i32     ; <i32> [#uses=1]
  %9 = sitofp i32 %storemerge243 to double        ; <double> [#uses=1]
  br label %bb13

bb13:                                             ; preds = %bb13, %bb.nph42
  %indvar90 = phi i64 [ 0, %bb.nph42 ], [ %indvar.next91, %bb13 ] ; <i64> [#uses=3]
  %scevgep95 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar93, i64 %indvar90 ; <double*> [#uses=1]
  %storemerge1141 = trunc i64 %indvar90 to i32    ; <i32> [#uses=1]
  %10 = sitofp i32 %storemerge1141 to double      ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep95, align 8
  %indvar.next91 = add i64 %indvar90, 1           ; <i64> [#uses=2]
  %exitcond92 = icmp eq i64 %indvar.next91, 512   ; <i1> [#uses=1]
  br i1 %exitcond92, label %bb15, label %bb13

bb15:                                             ; preds = %bb13
  %indvar.next94 = add i64 %indvar93, 1           ; <i64> [#uses=2]
  %exitcond96 = icmp eq i64 %indvar.next94, 512   ; <i1> [#uses=1]
  br i1 %exitcond96, label %bb.nph35, label %bb.nph42

bb.nph35:                                         ; preds = %bb21, %bb15
  %indvar85 = phi i64 [ %indvar.next86, %bb21 ], [ 0, %bb15 ] ; <i64> [#uses=3]
  %storemerge336 = trunc i64 %indvar85 to i32     ; <i32> [#uses=1]
  %14 = sitofp i32 %storemerge336 to double       ; <double> [#uses=1]
  br label %bb19

bb19:                                             ; preds = %bb19, %bb.nph35
  %indvar82 = phi i64 [ 0, %bb.nph35 ], [ %indvar.next83, %bb19 ] ; <i64> [#uses=3]
  %scevgep87 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar85, i64 %indvar82 ; <double*> [#uses=1]
  %storemerge1034 = trunc i64 %indvar82 to i32    ; <i32> [#uses=1]
  %15 = sitofp i32 %storemerge1034 to double      ; <double> [#uses=1]
  %16 = fmul double %14, %15                      ; <double> [#uses=1]
  %17 = fadd double %16, 2.000000e+00             ; <double> [#uses=1]
  %18 = fdiv double %17, 5.120000e+02             ; <double> [#uses=1]
  store double %18, double* %scevgep87, align 8
  %indvar.next83 = add i64 %indvar82, 1           ; <i64> [#uses=2]
  %exitcond84 = icmp eq i64 %indvar.next83, 512   ; <i1> [#uses=1]
  br i1 %exitcond84, label %bb21, label %bb19

bb21:                                             ; preds = %bb19
  %indvar.next86 = add i64 %indvar85, 1           ; <i64> [#uses=2]
  %exitcond88 = icmp eq i64 %indvar.next86, 512   ; <i1> [#uses=1]
  br i1 %exitcond88, label %bb.nph28, label %bb.nph35

bb.nph28:                                         ; preds = %bb27, %bb21
  %indvar77 = phi i64 [ %indvar.next78, %bb27 ], [ 0, %bb21 ] ; <i64> [#uses=3]
  %storemerge429 = trunc i64 %indvar77 to i32     ; <i32> [#uses=1]
  %19 = sitofp i32 %storemerge429 to double       ; <double> [#uses=1]
  br label %bb25

bb25:                                             ; preds = %bb25, %bb.nph28
  %indvar74 = phi i64 [ 0, %bb.nph28 ], [ %indvar.next75, %bb25 ] ; <i64> [#uses=3]
  %scevgep79 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar77, i64 %indvar74 ; <double*> [#uses=1]
  %storemerge927 = trunc i64 %indvar74 to i32     ; <i32> [#uses=1]
  %20 = sitofp i32 %storemerge927 to double       ; <double> [#uses=1]
  %21 = fmul double %19, %20                      ; <double> [#uses=1]
  %22 = fadd double %21, 2.000000e+00             ; <double> [#uses=1]
  %23 = fdiv double %22, 5.120000e+02             ; <double> [#uses=1]
  store double %23, double* %scevgep79, align 8
  %indvar.next75 = add i64 %indvar74, 1           ; <i64> [#uses=2]
  %exitcond76 = icmp eq i64 %indvar.next75, 512   ; <i1> [#uses=1]
  br i1 %exitcond76, label %bb27, label %bb25

bb27:                                             ; preds = %bb25
  %indvar.next78 = add i64 %indvar77, 1           ; <i64> [#uses=2]
  %exitcond80 = icmp eq i64 %indvar.next78, 512   ; <i1> [#uses=1]
  br i1 %exitcond80, label %bb.nph21, label %bb.nph28

bb.nph21:                                         ; preds = %bb33, %bb27
  %indvar69 = phi i64 [ %indvar.next70, %bb33 ], [ 0, %bb27 ] ; <i64> [#uses=3]
  %storemerge522 = trunc i64 %indvar69 to i32     ; <i32> [#uses=1]
  %24 = sitofp i32 %storemerge522 to double       ; <double> [#uses=1]
  br label %bb31

bb31:                                             ; preds = %bb31, %bb.nph21
  %indvar66 = phi i64 [ 0, %bb.nph21 ], [ %indvar.next67, %bb31 ] ; <i64> [#uses=3]
  %scevgep71 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %indvar69, i64 %indvar66 ; <double*> [#uses=1]
  %storemerge820 = trunc i64 %indvar66 to i32     ; <i32> [#uses=1]
  %25 = sitofp i32 %storemerge820 to double       ; <double> [#uses=1]
  %26 = fmul double %24, %25                      ; <double> [#uses=1]
  %27 = fadd double %26, 2.000000e+00             ; <double> [#uses=1]
  %28 = fdiv double %27, 5.120000e+02             ; <double> [#uses=1]
  store double %28, double* %scevgep71, align 8
  %indvar.next67 = add i64 %indvar66, 1           ; <i64> [#uses=2]
  %exitcond68 = icmp eq i64 %indvar.next67, 512   ; <i1> [#uses=1]
  br i1 %exitcond68, label %bb33, label %bb31

bb33:                                             ; preds = %bb31
  %indvar.next70 = add i64 %indvar69, 1           ; <i64> [#uses=2]
  %exitcond72 = icmp eq i64 %indvar.next70, 512   ; <i1> [#uses=1]
  br i1 %exitcond72, label %bb.nph, label %bb.nph21

bb.nph:                                           ; preds = %bb39, %bb33
  %indvar62 = phi i64 [ %indvar.next63, %bb39 ], [ 0, %bb33 ] ; <i64> [#uses=3]
  %storemerge615 = trunc i64 %indvar62 to i32     ; <i32> [#uses=1]
  %29 = sitofp i32 %storemerge615 to double       ; <double> [#uses=1]
  br label %bb37

bb37:                                             ; preds = %bb37, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb37 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %indvar62, i64 %indvar ; <double*> [#uses=1]
  %storemerge714 = trunc i64 %indvar to i32       ; <i32> [#uses=1]
  %30 = sitofp i32 %storemerge714 to double       ; <double> [#uses=1]
  %31 = fmul double %29, %30                      ; <double> [#uses=1]
  %32 = fadd double %31, 2.000000e+00             ; <double> [#uses=1]
  %33 = fdiv double %32, 5.120000e+02             ; <double> [#uses=1]
  store double %33, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb39, label %bb37

bb39:                                             ; preds = %bb37
  %indvar.next63 = add i64 %indvar62, 1           ; <i64> [#uses=2]
  %exitcond64 = icmp eq i64 %indvar.next63, 512   ; <i1> [#uses=1]
  br i1 %exitcond64, label %return, label %bb.nph

return:                                           ; preds = %bb39
  ret void
}

define void @print_array(i32 %argc, i8** %argv) nounwind inlinehint {
entry:
  %0 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %0, label %bb, label %return

bb:                                               ; preds = %entry
  %1 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %2 = load i8* %1, align 1                       ; <i8> [#uses=1]
  %3 = icmp eq i8 %2, 0                           ; <i1> [#uses=1]
  br i1 %3, label %bb.nph.us, label %return

bb7.us:                                           ; preds = %bb5.us
  %4 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %5 = bitcast %struct._IO_FILE* %4 to i8*        ; <i8*> [#uses=1]
  %6 = tail call i32 @fputc(i32 10, i8* %5) nounwind ; <i32> [#uses=0]
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9, 512    ; <i1> [#uses=1]
  br i1 %exitcond12, label %return, label %bb.nph.us

bb5.us:                                           ; preds = %bb4.us, %bb3.us
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us, label %bb3.us

bb3.us:                                           ; preds = %bb.nph.us, %bb5.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb5.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %tmp15 = add i64 %tmp14, %indvar                ; <i64> [#uses=1]
  %tmp11 = trunc i64 %tmp15 to i32                ; <i32> [#uses=1]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %tmp11, 80                       ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb4.us, label %bb5.us

bb4.us:                                           ; preds = %bb3.us
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb5.us

bb.nph.us:                                        ; preds = %bb7.us, %bb
  %indvar8 = phi i64 [ %indvar.next9, %bb7.us ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp14 = shl i64 %indvar8, 9                    ; <i64> [#uses=1]
  br label %bb3.us

return:                                           ; preds = %bb7.us, %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func(i64 %ni, i64 %nj, i64 %nk, i64 %nl, i64 %nm) nounwind {
entry:
  %0 = icmp sgt i64 %ni, 0                        ; <i1> [#uses=3]
  br i1 %0, label %bb.nph76.bb.nph76.split_crit_edge, label %return

bb.nph62:                                         ; preds = %bb.nph76.bb.nph76.split_crit_edge, %bb6
  %storemerge63 = phi i64 [ 0, %bb.nph76.bb.nph76.split_crit_edge ], [ %7, %bb6 ] ; <i64> [#uses=4]
  br i1 %9, label %bb.nph54.us, label %bb4

bb4.us:                                           ; preds = %bb2.us
  store double %5, double* %scevgep105
  %1 = add nsw i64 %storemerge758.us, 1           ; <i64> [#uses=2]
  %exitcond103 = icmp eq i64 %1, %ni              ; <i1> [#uses=1]
  br i1 %exitcond103, label %bb6, label %bb.nph54.us

bb2.us:                                           ; preds = %bb.nph54.us, %bb2.us
  %.tmp.056.us = phi double [ 0.000000e+00, %bb.nph54.us ], [ %5, %bb2.us ] ; <double> [#uses=1]
  %storemerge853.us = phi i64 [ 0, %bb.nph54.us ], [ %6, %bb2.us ] ; <i64> [#uses=3]
  %scevgep101 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge853.us, i64 %storemerge758.us ; <double*> [#uses=1]
  %scevgep102 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge63, i64 %storemerge853.us ; <double*> [#uses=1]
  %2 = load double* %scevgep102, align 8          ; <double> [#uses=1]
  %3 = load double* %scevgep101, align 8          ; <double> [#uses=1]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %.tmp.056.us, %4               ; <double> [#uses=2]
  %6 = add nsw i64 %storemerge853.us, 1           ; <i64> [#uses=2]
  %exitcond100 = icmp eq i64 %6, %nk              ; <i1> [#uses=1]
  br i1 %exitcond100, label %bb4.us, label %bb2.us

bb.nph54.us:                                      ; preds = %bb4.us, %bb.nph62
  %storemerge758.us = phi i64 [ %1, %bb4.us ], [ 0, %bb.nph62 ] ; <i64> [#uses=3]
  %scevgep105 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge63, i64 %storemerge758.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep105, align 8
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph62
  %indvar108 = phi i64 [ %indvar.next109, %bb4 ], [ 0, %bb.nph62 ] ; <i64> [#uses=2]
  %storemerge70 = phi i64 [ %storemerge70, %bb4 ], [ %storemerge63, %bb.nph62 ] ; <i64> [#uses=3]
  %tmp112 = shl i64 %storemerge70, 9              ; <i64> [#uses=1]
  %scevgep111.sum = add i64 %indvar108, %tmp112   ; <i64> [#uses=1]
  %scevgep113 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 0, i64 %scevgep111.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep113, align 8
  %indvar.next109 = add i64 %indvar108, 1         ; <i64> [#uses=2]
  %exitcond110 = icmp eq i64 %indvar.next109, %ni ; <i1> [#uses=1]
  br i1 %exitcond110, label %bb6, label %bb4

bb6:                                              ; preds = %bb4, %bb4.us
  %storemerge75 = phi i64 [ %storemerge63, %bb4.us ], [ %storemerge70, %bb4 ] ; <i64> [#uses=1]
  %7 = add nsw i64 %storemerge75, 1               ; <i64> [#uses=2]
  %8 = icmp slt i64 %7, %ni                       ; <i1> [#uses=1]
  br i1 %8, label %bb.nph62, label %bb16.preheader

bb.nph76.bb.nph76.split_crit_edge:                ; preds = %entry
  %9 = icmp sgt i64 %nk, 0                        ; <i1> [#uses=1]
  br label %bb.nph62

bb16.preheader:                                   ; preds = %bb6
  br i1 %0, label %bb.nph52.bb.nph52.split_crit_edge, label %return

bb.nph38:                                         ; preds = %bb.nph52.bb.nph52.split_crit_edge, %bb15
  %storemerge139 = phi i64 [ 0, %bb.nph52.bb.nph52.split_crit_edge ], [ %16, %bb15 ] ; <i64> [#uses=4]
  br i1 %18, label %bb.nph30.us, label %bb13

bb13.us:                                          ; preds = %bb11.us
  store double %14, double* %scevgep90
  %10 = add nsw i64 %storemerge534.us, 1          ; <i64> [#uses=2]
  %exitcond88 = icmp eq i64 %10, %ni              ; <i1> [#uses=1]
  br i1 %exitcond88, label %bb15, label %bb.nph30.us

bb11.us:                                          ; preds = %bb.nph30.us, %bb11.us
  %.tmp.032.us = phi double [ 0.000000e+00, %bb.nph30.us ], [ %14, %bb11.us ] ; <double> [#uses=1]
  %storemerge629.us = phi i64 [ 0, %bb.nph30.us ], [ %15, %bb11.us ] ; <i64> [#uses=3]
  %scevgep86 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge629.us, i64 %storemerge534.us ; <double*> [#uses=1]
  %scevgep87 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge139, i64 %storemerge629.us ; <double*> [#uses=1]
  %11 = load double* %scevgep87, align 8          ; <double> [#uses=1]
  %12 = load double* %scevgep86, align 8          ; <double> [#uses=1]
  %13 = fmul double %11, %12                      ; <double> [#uses=1]
  %14 = fadd double %.tmp.032.us, %13             ; <double> [#uses=2]
  %15 = add nsw i64 %storemerge629.us, 1          ; <i64> [#uses=2]
  %exitcond85 = icmp eq i64 %15, %nk              ; <i1> [#uses=1]
  br i1 %exitcond85, label %bb13.us, label %bb11.us

bb.nph30.us:                                      ; preds = %bb13.us, %bb.nph38
  %storemerge534.us = phi i64 [ %10, %bb13.us ], [ 0, %bb.nph38 ] ; <i64> [#uses=3]
  %scevgep90 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %storemerge139, i64 %storemerge534.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep90, align 8
  br label %bb11.us

bb13:                                             ; preds = %bb13, %bb.nph38
  %indvar93 = phi i64 [ %indvar.next94, %bb13 ], [ 0, %bb.nph38 ] ; <i64> [#uses=2]
  %storemerge146 = phi i64 [ %storemerge146, %bb13 ], [ %storemerge139, %bb.nph38 ] ; <i64> [#uses=3]
  %tmp97 = shl i64 %storemerge146, 9              ; <i64> [#uses=1]
  %scevgep96.sum = add i64 %indvar93, %tmp97      ; <i64> [#uses=1]
  %scevgep98 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 0, i64 %scevgep96.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep98, align 8
  %indvar.next94 = add i64 %indvar93, 1           ; <i64> [#uses=2]
  %exitcond95 = icmp eq i64 %indvar.next94, %ni   ; <i1> [#uses=1]
  br i1 %exitcond95, label %bb15, label %bb13

bb15:                                             ; preds = %bb13, %bb13.us
  %storemerge151 = phi i64 [ %storemerge139, %bb13.us ], [ %storemerge146, %bb13 ] ; <i64> [#uses=1]
  %16 = add nsw i64 %storemerge151, 1             ; <i64> [#uses=2]
  %17 = icmp slt i64 %16, %ni                     ; <i1> [#uses=1]
  br i1 %17, label %bb.nph38, label %bb25.preheader

bb.nph52.bb.nph52.split_crit_edge:                ; preds = %bb16.preheader
  %18 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  br label %bb.nph38

bb25.preheader:                                   ; preds = %bb15
  br i1 %0, label %bb.nph28.bb.nph28.split_crit_edge, label %return

bb.nph14:                                         ; preds = %bb.nph28.bb.nph28.split_crit_edge, %bb24
  %storemerge215 = phi i64 [ 0, %bb.nph28.bb.nph28.split_crit_edge ], [ %25, %bb24 ] ; <i64> [#uses=4]
  br i1 %27, label %bb.nph.us, label %bb22

bb22.us:                                          ; preds = %bb20.us
  store double %23, double* %scevgep80
  %19 = add nsw i64 %storemerge310.us, 1          ; <i64> [#uses=2]
  %exitcond78 = icmp eq i64 %19, %ni              ; <i1> [#uses=1]
  br i1 %exitcond78, label %bb24, label %bb.nph.us

bb20.us:                                          ; preds = %bb.nph.us, %bb20.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %23, %bb20.us ] ; <double> [#uses=1]
  %storemerge49.us = phi i64 [ 0, %bb.nph.us ], [ %24, %bb20.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %storemerge49.us, i64 %storemerge310.us ; <double*> [#uses=1]
  %scevgep77 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge215, i64 %storemerge49.us ; <double*> [#uses=1]
  %20 = load double* %scevgep77, align 8          ; <double> [#uses=1]
  %21 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %22 = fmul double %20, %21                      ; <double> [#uses=1]
  %23 = fadd double %.tmp.0.us, %22               ; <double> [#uses=2]
  %24 = add nsw i64 %storemerge49.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %24, %nk                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb22.us, label %bb20.us

bb.nph.us:                                        ; preds = %bb22.us, %bb.nph14
  %storemerge310.us = phi i64 [ %19, %bb22.us ], [ 0, %bb.nph14 ] ; <i64> [#uses=3]
  %scevgep80 = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %storemerge215, i64 %storemerge310.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep80, align 8
  br label %bb20.us

bb22:                                             ; preds = %bb22, %bb.nph14
  %indvar = phi i64 [ %indvar.next, %bb22 ], [ 0, %bb.nph14 ] ; <i64> [#uses=2]
  %storemerge222 = phi i64 [ %storemerge222, %bb22 ], [ %storemerge215, %bb.nph14 ] ; <i64> [#uses=3]
  %tmp = shl i64 %storemerge222, 9                ; <i64> [#uses=1]
  %scevgep82.sum = add i64 %indvar, %tmp          ; <i64> [#uses=1]
  %scevgep83 = getelementptr [512 x [512 x double]]* @G, i64 0, i64 0, i64 %scevgep82.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep83, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond81 = icmp eq i64 %indvar.next, %ni     ; <i1> [#uses=1]
  br i1 %exitcond81, label %bb24, label %bb22

bb24:                                             ; preds = %bb22, %bb22.us
  %storemerge227 = phi i64 [ %storemerge215, %bb22.us ], [ %storemerge222, %bb22 ] ; <i64> [#uses=1]
  %25 = add nsw i64 %storemerge227, 1             ; <i64> [#uses=2]
  %26 = icmp slt i64 %25, %ni                     ; <i1> [#uses=1]
  br i1 %26, label %bb.nph14, label %return

bb.nph28.bb.nph28.split_crit_edge:                ; preds = %bb25.preheader
  %27 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  br label %bb.nph14

return:                                           ; preds = %bb24, %bb25.preheader, %bb16.preheader, %entry
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  tail call void @init_array() nounwind
  tail call void @scop_func(i64 512, i64 512, i64 512, i64 512, i64 512) nounwind
  %0 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %0, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %entry
  %1 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %2 = load i8* %1, align 1                       ; <i8> [#uses=1]
  %3 = icmp eq i8 %2, 0                           ; <i1> [#uses=1]
  br i1 %3, label %bb.nph.us.i, label %print_array.exit

bb7.us.i:                                         ; preds = %bb5.us.i
  %4 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %5 = bitcast %struct._IO_FILE* %4 to i8*        ; <i8*> [#uses=1]
  %6 = tail call i32 @fputc(i32 10, i8* %5) nounwind ; <i32> [#uses=0]
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond3 = icmp eq i64 %indvar.next9.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond3, label %print_array.exit, label %bb.nph.us.i

bb5.us.i:                                         ; preds = %bb4.us.i, %bb3.us.i
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i, 512     ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb5.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %tmp5 = add i64 %tmp4, %indvar.i                ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp5 to i32               ; <i32> [#uses=1]
  %7 = load double* %scevgep.i, align 8           ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb4.us.i, label %bb5.us.i

bb4.us.i:                                         ; preds = %bb3.us.i
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i:                                      ; preds = %bb7.us.i, %bb.i
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp4 = shl i64 %indvar8.i, 9                   ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %entry
  ret i32 0
}
