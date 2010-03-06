open Genlex
open Printf
open Unix
open Str
open ExtString
open ExtList
open Std

let (|>) f x = x f

exception Bye
exception ThreadExit

type events = EWaitAnyChar of float | EWaitAnswer of float 
            | ETimeOfDay of float   | EAnsw | ETimeout | EAbortThread
            | ESetEcho of bool | EPrintString of string
            | ERunFilter of string | EKillFilter of int
            | EEatInput | EWaitSome

type token = Raw of string | Directive of string

type opts  = {
               mutable opt_port     : string;
               mutable opt_baudrate : int;
               mutable opt_script   : string 
             }

type processing_opts = {
    proc_echo : bool;
    proc_filt: int option;
    proc_filt_in: Pervasives.out_channel option;
    proc_filt_out: Pervasives.in_channel option;
}


let fl = Pervasives.flush

let get_args () =

   let opts = { 
                opt_port     = "/dev/ttyUSB0";
                opt_baudrate = 230400;
                opt_script   = "" 
              }

    in let cmdo = [("--port", Arg.String(fun v -> opts.opt_port <- v), (sprintf "port, default: %s" opts.opt_port));
                   ("--baudrate",Arg.Int(fun v -> opts.opt_baudrate <- v), (sprintf "baudrate, default: %d" opts.opt_baudrate));
                 ]
    in let _ = Arg.parse cmdo (fun x -> opts.opt_script <- x) "Usage:"
    in opts

let script opts = 
    let inp = Pervasives.open_in opts.opt_script
    in Std.input_chars inp

let port opts =
  let fd = openfile opts.opt_port [O_RDWR] 0
  in let ta = tcgetattr fd
  in let ta' = { ta with
                 c_obaud = opts.opt_baudrate;
                 c_ibaud = opts.opt_baudrate;
                 c_istrip = false;
                 c_inlcr = false;
                 c_igncr = false;
                 c_icrnl = false;
                 c_ixoff = false;
                 c_csize = 8;
                 c_cstopb = 1;
                 c_cread = true;
                 c_parenb = false;
                 c_hupcl = true;
                 c_clocal = false;
                 c_isig = false;
                 c_icanon = false;
                 c_noflsh = true;
                 c_echo = false;
                 c_echoe = false;
                 c_echok = false;
                 c_echonl = false;
                 c_vmin = 1024;
                 c_vtime = 1;
	    } 
  in let _ = tcsetattr fd TCSAFLUSH ta';
  in let _ = tcflush fd TCIOFLUSH
  in let ic = in_channel_of_descr fd
  in let oc = out_channel_of_descr fd
  in (ic, oc, fd)

let rec logger () = 
    Thread.yield(); logger()

let dump_chars = List.iter (fun x -> Pervasives.output_char Pervasives.stdout x)

let rec tokenize chars tok tokens = 
    let add_token x toks = match x with
      | x::xs -> Raw((String.implode (List.rev tok))) :: toks
      | []    -> toks
    in let get_directive chars = 
        let tok = List.takewhile (fun c -> ((c != '\r') && (c != '\n')) ) chars
        in (tok, List.drop (List.length tok) chars)
    in match chars with 
      | '\r' :: xs
      | '\n' :: xs
      | '\t' :: xs
      | ' '  :: xs -> tokenize xs [] (add_token tok tokens)
      | '%'  :: xs -> let t, rest = get_directive xs
                      in tokenize rest [] (Directive((String.implode ('%'::t))) :: tokens)
      | x    :: xs -> tokenize xs (x :: tok) tokens
      | []         -> List.rev (add_token tok tokens)


let send_string o s = Pervasives.output_string o s;
                      Pervasives.output_char o '\n';
                      Pervasives.flush o

let read_all i = 
    let rec readc chars =
        try readc chars @[Pervasives.input_char i]
        with End_of_file -> chars
    in String.implode (List.rev (readc []))

let read_data i timeout = 
    let t1 = Unix.gettimeofday ()
    in match (Unix.select [(descr_of_in_channel i)] [] [] timeout) with
    | (x::_,_,_) -> print_endline (read_all i) 
    | _          -> ()

let run_script opts (inp,outp) script  =
    let rec play_sequence opts c = match c with
    | Raw(s)       :: rest  -> send_string outp s; read_data inp 0.01; play_sequence opts rest
    | Directive(s) :: rest  -> (); play_sequence opts rest
    | []                    -> ()

    in let def_opts = { proc_filt    = None; proc_filt_in = None;
                       proc_filt_out = None; proc_echo = true }

    in play_sequence def_opts (tokenize (List.of_enum script) [] [])

let with_io (i,o,fd) f = 
    let close_all () = close fd
    in try f (); close_all ();
       with x -> close_all (); raise x

let _ = 
    let opts = get_args ()
    in let _ = printf "Running %s %d %s\n\n" opts.opt_port opts.opt_baudrate opts.opt_script
    in try  
        let (i,o,f) = port opts 
        in with_io (i,o,f) (fun () -> run_script opts (i,o) (script opts))
       with Bye -> print_endline "Bye!"

