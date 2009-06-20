%{
open Ti_txt
%}

%token <int> HEX 
%token Q
%token AT 
%token EOF

%start toplevel
%type <Ti_txt.ti_txt> toplevel

%%

toplevel:
    | EOF            { [] }
    | blocks Q       { $1 }

blocks:
    | block          { $1 :: [] }
    | block blocks   { $1 :: $2 }

block:
    | address data   { { b_addr = $1; b_data = $2 } }

address:
    | AT HEX         { $2 }

data:                { [] }
    | HEX data       { $1 :: $2 }

%%

