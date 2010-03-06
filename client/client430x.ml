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
                 c_vmin = 4096;
                 c_vtime = 5;
	    } 
  in let _ = tcsetattr fd TCSAFLUSH ta';
  in let _ = tcflush fd TCIOFLUSH
  in let ic = in_channel_of_descr fd
  in let oc = out_channel_of_descr fd
  in (ic, oc, fd)

let rec logger () = 
    Thread.yield(); logger()

let rec tokenize chars tok tokens = 
    let add_token x toks = match x with
      | x::xs -> (String.implode (List.rev tok)) :: toks
      | []    -> toks
    in match chars with 
      | '\r' :: xs
      | '\n' :: xs
      | '\t' :: xs
      | ' '  :: xs -> tokenize xs [] (add_token tok tokens)
      | '%'  :: xs -> let t, rest = List.partition (fun c -> c != '\r' and c != '\n') xs 
      | x    :: xs -> tokenize xs (x :: tok) tokens
      | []         -> List.rev (add_token tok tokens)

let run_script opts (inp,outp) script  =
    let play_sequence c = ()
    in let _ = List.iter print_endline (tokenize (List.of_enum script) [] [])
    in play_sequence (List.of_enum script) 

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

