; ModuleID = 'test4.c'
source_filename = "test4.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @casted(i32 noundef %a) #0 {
entry:
  %a.addr = alloca i32, align 4
  %x = alloca i32, align 4
  %p = alloca i8*, align 8
  %q = alloca i32*, align 8
  store i32 %a, i32* %a.addr, align 4
  %0 = bitcast i32* %x to i8*
  store i8* %0, i8** %p, align 8
  %1 = load i8*, i8** %p, align 8
  %2 = bitcast i8* %1 to i32*
  store i32* %2, i32** %q, align 8
  %3 = load i32, i32* %a.addr, align 4
  %4 = load i32*, i32** %q, align 8
  store i32 %3, i32* %4, align 4
  %5 = load i32*, i32** %q, align 8
  %6 = load i32, i32* %5, align 4
  ret i32 %6
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
