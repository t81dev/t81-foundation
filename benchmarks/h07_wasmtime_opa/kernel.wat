(module
  (func (export "compute") (param $n i32) (result i32)
    (local $i i32)
    (local $acc i32)
    (loop $L
      (local.set $acc
        (i32.rem_u
          (i32.add
            (local.get $acc)
            (i32.add
              (i32.mul (local.get $i) (i32.const 3))
              (i32.const 1)))
          (i32.const 65521)))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br_if $L (i32.lt_u (local.get $i) (local.get $n)))
    )
    (local.get $acc)
  )
)
