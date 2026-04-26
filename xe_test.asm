XETEST   START   0
FIRST    LDB     #100
         BASE    100
         CLEAR   A
         CLEAR   X
LOOP     ADD     #5
         TIX     MAX
         JLT     LOOP
         STA     TOTAL
         RSUB
MAX      WORD    50
TOTAL    RESW    1
         END     FIRST
