\ Core.fth - core word library for CFirth
\ Copyright 2022 Mark Seminatore. All rights reserved.

( n1 n2 -- n2 )
: NIP SWAP DROP ;

( --  c)
32 CONSTANT BL

( -- )
: SPACE BL EMIT ;
: LF 10 EMIT ;
: CR 13 EMIT 10 EMIT ;
: TAB 9 EMIT ;
: BEL 7 EMIT ;
: NEWLINE CR LF ;
: ESC 27 EMIT ;

\ toggles for hex/decimal output
: HEX 1 ENV.HEX ! ;
: DECIMAL 0 ENV.HEX ! ;

\ define a convenience word for printing the TOS ( n -- )
: PRINT . ;

\ sqr the TOS ( n -- n )
: SQR DUP * ;

\ leave the current code pointer on the stack
\ ( -- n )
: HERE CP @ ;

( n -- n )
\ : CELLS 4 * ;

( -- )
: CHARS ;

( n1 n2 -- n1 n2 n1 n2 )
: 2DUP OVER OVER ;

( n1 n2 -- )
: 2DROP DROP DROP ;

( -- )
: RDROP R> DROP ;

( -- n1 n2 ) ( r: n1 n2 -- )
: 2R> R> R> ;

( n1 n2 -- ) ( r: -- n1 n2 )
: 2>R >R >R ;

( n1 n2 -- n2 n1 n2 )
: TUCK SWAP OVER ;

\ : IF ' BRANCH? , HERE 0 , ; IMMEDIATE

\ : THEN HERE SWAP ! ; IMMEDIATE

( n1 -- 0 | n1 n1 )
: ?DUP DUP DUP 0= IF DROP THEN ;

( n1 n2 -- n1|n2 ) 
: MIN OVER OVER - 0< IF DROP ELSE NIP THEN ;

( n1 n2 -- n1|n2 )
: MAX OVER OVER - 0> IF DROP ELSE NIP THEN ;

( n -- -n )
: NEGATE -1 * ;

( n -- |n| )
: ABS DUP 0< IF -1 * THEN ;

( n -- n++ )
: 1+ 1 + ;

( n -- n-- )
: 1- 1 - ;

\ boolean values
0 CONSTANT FALSE
-1 CONSTANT TRUE

\ number ranges
2147483647 CONSTANT MAX-INT
-2147483646 CONSTANT MIN-INT
2147483648 CONSTANT MSB

\ : IF BZ [ HERE ] ;

\ : THEN [ HERE SWAP ! ] ;
