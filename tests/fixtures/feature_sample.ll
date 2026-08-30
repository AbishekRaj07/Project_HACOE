source_filename = "feature_sample.c"
target triple = "x86_64-unknown-linux-gnu"

declare void @sink(i32)

define void @caller(i32 %value) {
entry:
  call void @sink(i32 %value)
  ret void
}

define i32 @sum(ptr %data, i32 %length) {
entry:
  %empty = icmp sle i32 %length, 0
  br i1 %empty, label %exit, label %loop

loop:
  %index = phi i32 [ 0, %entry ], [ %next_index, %loop ]
  %accumulator = phi i32 [ 0, %entry ], [ %next_sum, %loop ]
  %element_pointer = getelementptr inbounds i32, ptr %data, i32 %index
  %element = load i32, ptr %element_pointer, align 4
  %next_sum = add i32 %accumulator, %element
  %next_index = add i32 %index, 1
  %finished = icmp eq i32 %next_index, %length
  br i1 %finished, label %exit, label %loop

exit:
  %result = phi i32 [ 0, %entry ], [ %next_sum, %loop ]
  ret i32 %result
}
