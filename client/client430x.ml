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
    proc_read_timeout: float;
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
                      in tokenize rest [] (Directive((String.implode t)) :: tokens)
      | x    :: xs -> tokenize xs (x :: tok) tokens
      | []         -> List.rev (add_token tok tokens)


let send_string o s = Pervasives.output_string o s;
                      Pervasives.output_char o '\n';
                      Pervasives.flush o

let send_string_raw o s = Pervasives.output_string o s;
                          Pervasives.flush o

let read_all i = 
    let rec readc chars =
        try readc chars @[Pervasives.input_char i]
        with End_of_file -> chars
    in String.implode (List.rev (readc []))

let write_string opts s =
    if opts.proc_echo then 
    let channel = 
        match opts.proc_filt_in with 
        | Some(c) -> c
        | None    -> Pervasives.stdout
    in Pervasives.output_string channel s; fl channel
    else ()

let cmd_lexer = make_lexer ["read_timeout";"wait_input";"wait";"timeofday";"bye";
                            "echo"; "print";"run_filter";"kill_filter";
                            "sendstr"]


let run_script opts (inp,outp) script  =
    let rec play_sequence opts c = match c with
      | Raw(s)       :: rest  -> send_string outp s; read_data opts; play_sequence opts rest
      | Directive(s) :: rest  -> let opts' = process_directive opts s in play_sequence opts' rest
      | []                    -> ()
    and process_directive opts s = exec_cmd opts s
    and exec_cmd opts s = 
        let rec cmd_parser = parser 
        | [< 'Kwd "read_timeout"; 'Float f >]  -> { opts with proc_read_timeout = f }
        | [< 'Kwd "wait_input";   'Float f >]  -> read_data {opts with proc_read_timeout = f}; opts
        | [< 'Kwd "wait"; 'Float f >]          -> Thread.delay f; opts
        | [< 'Kwd "echo"; 'Int v  >]           -> { opts with proc_echo = v > 0  }
        | [< 'Kwd "print"; 'String v  >]       -> let () = write_string opts v in opts
        | [< 'Kwd "run_filter"; 'String v  >]  -> spawn_filter opts v
        | [< 'Kwd "kill_filter"; 'Int sg  >]   -> kill_filter opts sg 
        | [< 'Kwd "sendstr"; 'String s  >]     -> send_string_raw outp s; opts
        | [< 'Kwd "bye" >]                     -> kill_filter opts 15; raise Bye
        | [<>]                                 -> opts
        in s |> Stream.of_string |> cmd_lexer |> cmd_parser

    and spawn_filter opts p =
        let _ = kill_filter opts 15
        in let fin, fout = Unix.pipe ()
        in let c_in, c_out = (in_channel_of_descr fin, out_channel_of_descr fout)
        in let pid = Unix.create_process p [||] fin (descr_of_out_channel Pervasives.stdout)
                                                    (descr_of_out_channel Pervasives.stderr)
        in { opts with proc_filt = Some(pid); proc_filt_in = Some(c_out); proc_filt_out = Some(c_in) }

    and kill_filter opts s =
        let _ = match opts.proc_filt with 
                | Some(pid) -> Unix.kill pid s
                | None    -> ()
        in { opts with proc_filt = None; proc_filt_in = None; proc_filt_out = None }

    and read_data opts = 
    match (Unix.select [(descr_of_in_channel inp)] [] [] opts.proc_read_timeout) with
      | (x::_,_,_) -> write_string opts (read_all inp) ; Pervasives.flush_all ()
      | _          -> ()

    in let def_opts = { proc_read_timeout = 0.008;
                        proc_filt    = None; proc_filt_in = None;
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

