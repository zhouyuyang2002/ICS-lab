iaddq

Code: C 0 F Rb V

Fetch:
ifun:icode = M1[pc]
Ra:Rb = M1[pc+1]
ValC = M8[pc+2]
ValP = pc + 8

Decode:
ValB = R[Rb]

Eval:
ValE = ValB + ValC

Memory:


Writeback:
R[Rb] = ValE
PC = ValP





jm

Code: D 0 F Rb V

Fetch:
ifun:icode = M1[pc]
Ra:Rb = M1[pc+1]
ValC = M8[pc+2]
ValP = pc + 8

Decode:
ValB = R[Rb]

Eval:
ValE = ValB + ValC

Memory:
ValM = M8[ValE]

Writeback:
PC = ValM



