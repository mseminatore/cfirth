\ core-tests.fth - test the core Firth Words
\ Copyright 2022 Mark SeMINatore. All rights reserved.

\
\ Some test cases borrowed from https://forth-stANDard.org/stANDard/core/
\
include test.fth

Test-group
    ." Test DROP " CR
    T{ 1 2 DROP }T 1 ==

    ." Test SWAP " CR
    T{ 1 2 SWAP }T 2 1 ==

    ." Test DUP " CR
    T{ 1 DUP }T 1 1 ==

    ." Test MAX " CR
    T{ 1 2 MAX }T 2 ==
    T{ 2 1 MAX }T 2 ==
    T{ -2 -1 MAX }T -1 ==

    ." Test MIN " CR
    T{ 1 2 MIN }T 1 ==
    T{ 2 1 MIN }T 1 ==
    T{ -2 -1 MIN }T -2 ==

    ." Test NEGATE " CR
    T{ 1 NEGATE }T -1 ==
    T{ -1 NEGATE }T 1 ==
    T{ 0 NEGATE }T 0 ==

    ." Test ABS " CR
    T{ -1 ABS }T 1 ==
    T{ 1 ABS }T 1 ==
    T{ 0 ABS }T 0 ==

    ." Test NIP " CR
    T{ 1 2 NIP }T 2 ==

    ." Test NOT " CR
    T{ 0 NOT }T 1 ==
    T{ 1 NOT }T 0 ==

    ." Test OR " CR
    T{ 0 0 OR }T 0 ==
    T{ 1 0 OR }T 1 ==
    T{ 0 1 OR }T 1 ==
    T{ 1 1 OR }T 1 ==
    T{ 0 255 OR }T 255 ==

    ." Test XOR " CR
    T{ 0 0 XOR }T 0 ==
    T{ 0 1 XOR }T 1 ==
    T{ 1 0 XOR }T 1 ==
    T{ 1 1 XOR }T 0 ==

    ." Test 2DUP " CR
    T{ 1 2 2DUP }T 1 2 1 2 ==
    
    ." Test 2DROP " CR
    T{ 1 2 3 2DROP }T 1 ==

    ." Test ?DUP " CR
    T{ 1 ?DUP }T 1 1 ==
    T{ 0 ?DUP }T 0 ==

    ." Test */ " CR
    T{ 1 2 2 */ }T 1 ==
    T{ MAX-INT 2 MAX-INT */ }T 2 ==
    T{ MIN-INT 2 MIN-INT */ }T 2 ==

    ." Test < " CR
    T{ 1 2 < }T -1 ==
    T{ 2 1 < }T 0 ==
    T{ -1 1 < }T -1 ==

    ." Test > " CR
    T{ 1 2 > }T 0 ==
    T{ 2 1 > }T -1 ==
    T{ -1 1 > }T 0 ==

    ." Test = " CR
    T{ 1 1 = }T -1 ==
    T{ 1 0 = }T 0 ==

    ." Test <> " CR
    T{ 1 1 <> }T 0 ==
    T{ 1 0 <> }T -1 ==

    ." Test 0= " CR
    T{ 1 0= }T 0 ==
    T{ 0 0= }T -1 ==

    ." Test 0< " CR
    T{ 1 0< }T 0 ==
    T{ -1 0< }T -1 ==
    T{ 0 0< }T 0 ==

    ." Test 0> " CR
    T{ 1 0> }T -1 ==
    T{ -1 0> }T 0 ==
    T{ 0 0> }T 0 ==

    ." Test 0<> " CR
    T{ 1 0<> }T -1 ==
    T{ 0 0<> }T 0 ==
    
    ." Test AND " CR
    T{ 0 0 AND }T 0 ==
    T{ 1 0 AND }T 0 ==
    T{ 0 1 AND }T 0 ==
    T{ 1 1 AND }T 1 ==
    T{ 0 255 AND }T 0 ==

    ." Test OVER " CR
    T{ 1 2 over }T 1 2 1 ==

    ." Test POW " CR
    T{ 1 2 POW }T 1 ==
    T{ 2 2 POW }T 4 ==

    ." Test ROT " CR
    T{ 1 2 3 ROT }T 2 3 1 ==

    ." Test TUCK " CR
    T{ 1 2 TUCK }T 2 1 2 ==

    ." Test + " CR
    T{ 1 1 + }T 2 ==
    T{ 0 1 + }T 1 ==
    T{ -1 2 + }T 1 ==
    
    ." Test - " CR
    T{ 1 1 - }T 0 ==
    T{ 0 1 - }T -1 ==
    T{ -1 2 - }T -3 ==
    
    ." Test * " CR
    T{ 1 1 * }T 1 ==
    T{ 0 1 * }T 0 ==
    T{ -1 2 * }T -2 ==
    
    ." Test / " CR
    T{ 1 1 / }T 1 ==
    T{ 0 1 / }T 0 ==
    T{ 3 1 / }T 3 ==

    ." Test MOD " CR
    T{ 0 1 MOD }T 0 ==
    T{ 1 1 MOD }T 0 ==
    T{ 2 1 MOD }T 0 ==

    ." Test /MOD " CR
    T{ 3 2 /MOD }T 1 1 ==
    T{ 1 1 /MOD }T 1 0 ==
    T{ 2 1 /MOD }T 2 0 ==

    ." Test SQR " CR
    T{ 0 SQR }T 0 ==
    T{ 1 SQR }T 1 ==
    T{ 2 SQR }T 4 ==
    T{ -2 SQR }T 4 ==

    ." Test 1+ " CR
    T{ 0 1+ }T 1 ==
    T{ -1 1+ }T 0 ==
    T{ 1 1+ }T 2 ==

    ." Test 1- " CR
    T{ 0 1- }T -1 ==
    T{ -1 1- }T -2 ==
    T{ 1 1- }T 0 ==

    ." Test LSHIFT " CR
    T{ 1 0 LSHIFT }T 1 ==
    T{ 1 1 LSHIFT }T 2 ==
    T{ 1 2 LSHIFT }T 4 ==
    \ T{ MSB 1 LSHIFT }T 0 ==

    ." Test RSHIFT " CR
    T{ 1 0 RSHIFT }T 1 ==
    T{ 1 1 RSHIFT }T 0 ==
    T{ 2 1 RSHIFT }T 1 ==
    T{ 4 2 RSHIFT }T 1 ==
    \ T{ MSB 1 RSHIFT 2 * }T MSB ==

    ." Test DO LOOP " CR
    T{ func DL1 3 0 DO I LOOP ; DL1 }T 0 1 2 ==
    T{ func DL2 10 7 FOR I LOOP ; DL2 }T 7 8 9 ==

    ." Test FOR LOOP " CR
    T{ func DL3 3 0 FOR I LOOP ; DL3 }T 0 1 2 ==

    ." Test FOR +LOOP " CR
    T{ func DL4 6 0 FOR I 2 +LOOP ; DL4 }T 0 2 4 ==

    ." Test DEPTH " CR
    T{ 0 1 DEPTH }T 0 1 2 ==
    T{ 0 DEPTH }T 0 1 ==
    T{ DEPTH }T 0 ==

    ." Test IF ELSE THEN " CR
    func GI1 IF 123 THEN ;
    func GI2 IF 123 ELSE 234 THEN ;
    T{ 0 0 GI1 }T 0 ==
    T{ 1 GI1 }T 123 ==
    T{ -1 GI1 }T 123 ==
    T{ 0 GI2 }T 234 ==
    T{ 1 GI2 }T 123 ==
    T{ -1 GI2 }T 123 ==

    ." Test IF ELSE ENDIF " CR
    func GI3 IF 123 ENDIF ;
    func GI4 IF 123 ELSE 234 ENDIF ;
    T{ 0 0 GI3 }T 0 ==
    T{ 1 GI3 }T 123 ==
    T{ -1 GI3 }T 123 ==
    T{ 0 GI4 }T 234 ==
    T{ 1 GI4 }T 123 ==
    T{ -1 GI4 }T 123 ==

    ." Test >R R> " CR
    T{ 0 >R R> }T 0 ==
    T{ 1 2 3 >R R> }T 1 2 3 ==

    ." Test 2>R 2R> " CR
    T{ 0 1 2>R 2R> }T 0 1 ==
    T{ 1 2 3 2>R 2R> }T 1 2 3 ==

    ." Test CHARS " CR
    T{ 4 CHARS }T 4 ==
    T{ 1 CHARS }T 1 ==

    ." Test CELLS " CR
    T{ 4 CELLS }T ENV.CELL.SIZE 4 * ==
    T{ 1 CELLS }T ENV.CELL.SIZE ==

    ." Test RECURSE " CR
    func GI5 ( +n1 -- +n2) DUP 2 < IF DROP 1 EXIT THEN DUP 1- RECURSE * ;
    T{ 5 GI5 }T 120 ==
    T{ 4 GI5 }T 24 ==

    ." Test UNLOOP and EXIT " CR
    func GI6 10 0 DO I DUP 5 = IF UNLOOP EXIT ELSE DROP ENDIF LOOP ;
    T{ GI6 }T 5 == 

    ." Test BEGIN and UNTIL " CR
    func GI7 BEGIN DUP 1+ DUP 5 > UNTIL ;
    T{ 3 GI7 }T 3 4 5 6 ==
    T{ 5 GI7 }T 5 6 ==
    T{ 6 GI7 }T 6 7 ==

    ." Test WHILE and REPEAT " CR
    func GI8 BEGIN DUP 5 < WHILE DUP 1+ REPEAT ;
    T{ 0 GI8 }T 0 1 2 3 4 5 ==
    T{ 4 GI8 }T 4 5 ==
    T{ 5 GI8 }T 5 ==
    T{ 6 GI8 }T 6 ==

    ." Test AGAIN " CR
    func GI9 BEGIN 1+ DUP 10 = IF EXIT ENDIF AGAIN ;
    T{ 5 GI9 }T 10 ==
    T{ 0 GI9 }T 10 ==

    ." Test FILL " CR
    var foo
    T{ foo 1 CELLS 255 FILL foo @ }T -1 ==
    T{ foo 1 CELLS 0 FILL foo @ }T 0 ==

    ." Test ERASE " CR
    -1 foo !
    T{ foo 1 CELLS ERASE foo @ }T 0 ==
Test-end
