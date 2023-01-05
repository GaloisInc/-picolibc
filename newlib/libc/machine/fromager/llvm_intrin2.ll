source_filename = "../newlib/libc/machine/fromager/llvm_intrin.ll"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dso_local i32 @__llvm__usub__sat__i32(i32, i32) local_unnamed_addr #4 {
  %ge = icmp uge i32 %0, %1
  br i1 %ge, label %sub, label %zero

sub:
  %result = sub nuw i32 %0, %1
  ret i32 %result

zero:
  ret i32 0
}

attributes #4 = { norecurse nounwind optsize readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "prefer-vector-width"="1" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
