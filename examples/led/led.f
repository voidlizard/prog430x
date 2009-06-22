0 echo
1 led
aquire
%wait_input 0.2
reset
buf
$4031 !w+  $5C00 !w+  $40B2 !w+  $5A80 !w+  $015C !w+  $403F !w+  $0000 !w+  $930F !w+ 
$2405 !w+  $832F !w+  $4F9F !w+  $5C52 !w+  $1C00 !w+  $23FB !w+  $403F !w+  $0000 !w+ 
$930F !w+  $2404 !w+  $831F !w+  $43CF !w+  $1C00 !w+  $23FC !w+  $4030 !w+  $5C36 !w+ 
$4030 !w+  $5C34 !w+  $1300 !w+  $4031 !w+  $5C00 !w+  $40B2 !w+  $5A80 !w+  $015C !w+ 
$43F2 !w+  $0224 !w+  $43F2 !w+  $0222 !w+  $4303 !w+  $3FFE !w+  $4030 !w+  $5C50 !w+ 
$3FFF !w+  
$5C00 !xfe
%wait_input 0.3
$5C00 $0029 !xfwm
%wait_input 0.4

reset
buf
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+ 
$5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C30 !w+  $5C00 !w+ 

$FF80 !xfe
%wait_input 0.3
$FF80 $0040 !xfwm
%wait_input 0.4


0 led
release 
%wait_input 2.0

