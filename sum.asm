SUM      START   0
FIRST    LDA     FIVE
         ADD     TEN
         STA     RESULT
         RSUB
FIVE     WORD    5
TEN      WORD    10
RESULT   RESW    1
         END     FIRST
