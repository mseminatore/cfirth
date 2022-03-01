( *
  * LANGUAGE    : ANS Forth 
  * PROJECT     : Forth Environments
  * DESCRIPTION : The towers of Hanoi
  * CATEGORY    : Benchmarks
  * AUTHOR      : Peter Midnight, FORTH Dimensions Volume 2 #2.
  * LAST CHANGE : January 25th, 1992 MHX, made it 100% ANSI, used locals.
  * LAST CHANGE : April 15th, 1989 MHX
  * )

MARKER -hanoi	( "--- Towers of Hanoi     Version 2.00 ---" )

( ******* Utilities ******************************************** )

\ : ENDIF    POSTPONE THEN ; IMMEDIATE
\ : CURON  ; IMMEDIATE
\ : CUROFF ; IMMEDIATE

( ******* The looks ******************************************** )
( Change these for better appearance if your terminal allows it. )

CHAR - CONSTANT '-'
CHAR | CONSTANT '|'
CHAR " CONSTANT '"'

( ****** Main ************************************************** )

DECIMAL

0 VALUE N
3 VALUE #times

CREATE ring  14 ALLOT

: POS	  	N 2* 1+ * N + ; 		\ location pos -> coordinate

: HALFDISPLAY	0 DO  DUP EMIT			\ <color> <size> --- <>
		LOOP  DROP ; 

: <DISPLAY>	2DUP HALFDISPLAY  ROT		\ <line> <color> <size> --- <>
		3 < IF  BL 
		  ELSE  '|'
		 ENDIF  EMIT 
		HALFDISPLAY ; 

: DISPLAY					\ size pos line color ---
  		SWAP >R ROT ROT OVER - R@	\ color size pos-size line
		 AT-XY R>			\ color size line
		ROT ROT <DISPLAY> ; 

: PRESENCE	ring + C@ = NEGATE ; 		\ tower ring PRESENCE -> bool

: LINE		4 SWAP				\ tower line -> display-topline
		N 0 DO	
			DUP I PRESENCE 0= NEGATE 
			ROT + SWAP 
		  LOOP  
		DROP ; 


25 VALUE waits

: delay		waits MS ;


: RAISE		DUP POS SWAP LINE 		\ <size> <tower> --- <>
		2 SWAP DO  
			  2DUP I     BL  DISPLAY 
			  2DUP I 1-  '-' DISPLAY 
			  delay
		 -1 +LOOP 
		2DROP ; 

: LOWER		DUP POS SWAP  LINE 1+ 	 	\ size tower ---
		2 DO  
		     2DUP I 1- BL  DISPLAY 
		     2DUP I    '-' DISPLAY 
		     delay
		LOOP
		2DROP ; 

: MOVELEFT	POS  SWAP POS 1- 		\ size src.tower dest.tower ---
		      DO
			 DUP I 1+ 1  BL  DISPLAY
		   	 DUP I    1  '-' DISPLAY 
			 delay
		-1 +LOOP
		DROP ; 

: MOVERIGHT	POS 1+ SWAP POS 1+ 		\ size src.tower dest.tower ---
		    DO	
			DUP I 1- 1 BL  DISPLAY
			DUP I 1    '-' DISPLAY 
			delay
		  LOOP 
		DROP ; 

: TRAVERSE	2DUP > IF MOVELEFT 		\ size src.tower dest.tower ---
		     ELSE MOVERIGHT 
		    ENDIF ; 

: MOVE1		KEY? IF KEY BL <> 		\ size src.tower dest.tower ---
			   IF 0 N 4 + AT-XY ABORT 
			ENDIF 
		  ENDIF
		ROT ROT 2DUP RAISE 
		OVER SWAP 3 PICK TRAVERSE
		2DUP ring + 1- C! 
		SWAP LOWER ; 

: MULTIMOV	\ size source destiny spare ---
		LOCALS| spare destiny source size |
		size  1 = IF	size source destiny MOVE1
			ELSE	size 1- TO size
				size source spare destiny RECURSE
				size 1+ source destiny MOVE1
				size spare destiny source RECURSE
		       ENDIF ; 
 
: MAKETOWER	POS 4 N + 3 				\ tower ---
		DO  DUP I AT-XY  '|' EMIT LOOP 
		DROP ; 

: MAKEBASE	0 N 4 + AT-XY				\ ---
		N 6 * 3 + 0 
		DO  '"' EMIT LOOP ; 

: MAKERING	2DUP ring + 1- C! SWAP LOWER ; 		\ tower size ---

: SETUP 	PAGE					\ ---
		N 1+ 0 DO  1 ring I + C!    LOOP
		   3 0 DO  I MAKETOWER      LOOP
		MAKEBASE
		   1 N DO  0 I MAKERING -1 +LOOP 
		34 0 AT-XY ." *** iForth *** " ; 

: TOWERS	1 MAX 12 MIN TO N			\ quantity ---
		SETUP CUROFF
		N 2 0 1
	BEGIN	OVER POS N 4 + AT-XY
		\ STACK abcd|acdbacdb MULTIMOV
		2 ROLL 2OVER 2OVER    MULTIMOV
		#times 1- DUP TO #times
	UNTIL 	2DROP 2DROP
		0 N 6 + AT-XY  CURON ; 

: HTAB		CR SPACES ;		 		\ <xpos> --- <>

: .CE		DUP C@  80 SWAP - 2/ HTAB 		\ <'string> --- <>
		COUNT TYPE ; 

: HANOI		DEPTH 1 
	   < IF CR CR
		C" Hanoi expects the number of pieces on the stack." .CE CR
		C" For example, to solve a five piece towers of hanoi " .CE CR
		C" puzzle, type: " .CE CR CR
		C" 5 HANOI" .CE CR CR
		EXIT
	  ENDIF TOWERS ;

			    CR .( Type  HANOI)

			 ( * End of Information * )
