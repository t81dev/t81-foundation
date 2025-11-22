//===-- T81RegisterInfo.td - TISC Register Definitions -------*- tablegen -*-===//
//
// This file defines the general-purpose ternary registers for the T81/HanoiVM
// architecture backend.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

class T81Reg<string n, bits<16> num> : Register<n> {
  let HWEncoding = num;
}

//===----------------------------------------------------------------------===//
// General Purpose Registers (R0 - R80)
//===----------------------------------------------------------------------===//

def R0  : T81Reg<"R0", 0>;
def R1  : T81Reg<"R1", 1>;
def R2  : T81Reg<"R2", 2>;
def R3  : T81Reg<"R3", 3>;
def R4  : T81Reg<"R4", 4>;
def R5  : T81Reg<"R5", 5>;
def R6  : T81Reg<"R6", 6>;
def R7  : T81Reg<"R7", 7>;
def R8  : T81Reg<"R8", 8>;
def R9  : T81Reg<"R9", 9>;
def R10 : T81Reg<"R10", 10>;
def R11 : T81Reg<"R11", 11>;
def R12 : T81Reg<"R12", 12>;
def R13 : T81Reg<"R13", 13>;
def R14 : T81Reg<"R14", 14>;
def R15 : T81Reg<"R15", 15>;
def R16 : T81Reg<"R16", 16>;
def R17 : T81Reg<"R17", 17>;
def R18 : T81Reg<"R18", 18>;
def R19 : T81Reg<"R19", 19>;
def R20 : T81Reg<"R20", 20>;
def R21 : T81Reg<"R21", 21>;
def R22 : T81Reg<"R22", 22>;
def R23 : T81Reg<"R23", 23>;
def R24 : T81Reg<"R24", 24>;
def R25 : T81Reg<"R25", 25>;
def R26 : T81Reg<"R26", 26>;
def R27 : T81Reg<"R27", 27>;
def R28 : T81Reg<"R28", 28>;
def R29 : T81Reg<"R29", 29>;
def R30 : T81Reg<"R30", 30>;
def R31 : T81Reg<"R31", 31>;
def R32 : T81Reg<"R32", 32>;
def R33 : T81Reg<"R33", 33>;
def R34 : T81Reg<"R34", 34>;
def R35 : T81Reg<"R35", 35>;
def R36 : T81Reg<"R36", 36>;
def R37 : T81Reg<"R37", 37>;
def R38 : T81Reg<"R38", 38>;
def R39 : T81Reg<"R39", 39>;
def R40 : T81Reg<"R40", 40>;
def R41 : T81Reg<"R41", 41>;
def R42 : T81Reg<"R42", 42>;
def R43 : T81Reg<"R43", 43>;
def R44 : T81Reg<"R44", 44>;
def R45 : T81Reg<"R45", 45>;
def R46 : T81Reg<"R46", 46>;
def R47 : T81Reg<"R47", 47>;
def R48 : T81Reg<"R48", 48>;
def R49 : T81Reg<"R49", 49>;
def R50 : T81Reg<"R50", 50>;
def R51 : T81Reg<"R51", 51>;
def R52 : T81Reg<"R52", 52>;
def R53 : T81Reg<"R53", 53>;
def R54 : T81Reg<"R54", 54>;
def R55 : T81Reg<"R55", 55>;
def R56 : T81Reg<"R56", 56>;
def R57 : T81Reg<"R57", 57>;
def R58 : T81Reg<"R58", 58>;
def R59 : T81Reg<"R59", 59>;
def R60 : T81Reg<"R60", 60>;
def R61 : T81Reg<"R61", 61>;
def R62 : T81Reg<"R62", 62>;
def R63 : T81Reg<"R63", 63>;
def R64 : T81Reg<"R64", 64>;
def R65 : T81Reg<"R65", 65>;
def R66 : T81Reg<"R66", 66>;
def R67 : T81Reg<"R67", 67>;
def R68 : T81Reg<"R68", 68>;
def R69 : T81Reg<"R69", 69>;
def R70 : T81Reg<"R70", 70>;
def R71 : T81Reg<"R71", 71>;
def R72 : T81Reg<"R72", 72>;
def R73 : T81Reg<"R73", 73>;
def R74 : T81Reg<"R74", 74>;
def R75 : T81Reg<"R75", 75>;
def R76 : T81Reg<"R76", 76>;
def R77 : T81Reg<"R77", 77>;
def R78 : T81Reg<"R78", 78>;
def R79 : T81Reg<"R79", 79>;
def R80 : T81Reg<"R80", 80>;

//===----------------------------------------------------------------------===//
// General Purpose Register Class
//===----------------------------------------------------------------------===//

def T81GPR : RegisterClass<"T81", [i32], 32, 
  (add
    R0, R1, R2, R3, R4, R5, R6, R7, R8, R9,
    R10, R11, R12, R13, R14, R15, R16, R17, R18, R19,
    R20, R21, R22, R23, R24, R25, R26, R27, R28, R29,
    R30, R31, R32, R33, R34, R35, R36, R37, R38, R39,
    R40, R41, R42, R43, R44, R45, R46, R47, R48, R49,
    R50, R51, R52, R53, R54, R55, R56, R57, R58, R59,
    R60, R61, R62, R63, R64, R65, R66, R67, R68, R69,
    R70, R71, R72, R73, R74, R75, R76, R77, R78, R79,
    R80
  )> {
  let Size = 81;
}
