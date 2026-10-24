(module
  (import "host" "submit" (func $submit (param i32) (result i32)))
  (func (export "run") (param $units i32) (param $price i32) (result i32)
    local.get $units
    local.get $price
    i32.mul
    call $submit))
