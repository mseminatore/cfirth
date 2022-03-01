\ Colors.fth - terminal colors library for CFirth
\ Copyright 2022 Mark Seminatore. All rights reserved.

\ screen control
: TERM.CLEAR ESC ." [2J" ESC ." [H" ;
: PAGE TERM.CLEAR ;

\ not quite working yet, need to remove spaces
: AT-XY ESC ." [" . 59 EMIT . 72 EMIT ;

\ text colors
: TERM.RESET ESC ." [0m" ;
: TERM.BLACK ESC ." [30m" ;
: TERM.RED ESC ." [31m" ;
: TERM.GREEN ESC ." [32m" ;
: TERM.YELLOW ESC ." [33m" ;
: TERM.BLUE ESC ." [34m" ;
: TERM.MAGENTA ESC ." [35m" ;
: TERM.CYAN ESC ." [36m" ;
: TERM.WHITE ESC ." [37m" ;
