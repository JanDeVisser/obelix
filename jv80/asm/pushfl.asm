     mov a, #0x42
     mov b, #0x42
     clr c
     cmp a, b
     jnz hlt
     pushfl
     mov c, #0x37
     cmp a, c
     popfl
hlt: hlt
