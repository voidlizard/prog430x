{
  open Parser
  open Lexing
}

let hexdigit = ['0' - '9' 'a' - 'f' 'A' - 'F']
let hex      = hexdigit hexdigit*
let space    = [' ' '\t' '\r' '\n']

rule token = parse
	| space           { token lexbuf }
    | '@'             { AT }
    | 'q'             { Q }
    | 'Q'             { Q }
	| hex             { HEX (int_of_string( "0x" ^ lexeme lexbuf)) }


