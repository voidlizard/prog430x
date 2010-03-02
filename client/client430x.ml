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
            | ERunFilter of string

type opts  = {
               mutable opt_port     : string;
               mutable opt_baudrate : int;
               mutable opt_script   : string 
             }

type processing_opts = {
    proc_echo : bool;
    proc_filt:  (Pervasives.in_channel * Pervasives.out_channel) option
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
(*    split (regexp "[ \t\r\n]") (Std.input_file opts.opt_script)*)

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

let run_script opts (inp,outp) script  =
    let flush () = Pervasives.flush outp
    in let in_fd = descr_of_in_channel inp

    in let ch_a = Event.new_channel ()
    in let ch_b = Event.new_channel ()
    
    in let dump_out opts c =
        let oc = match opts.proc_filt with Some(i,o) -> o | None -> Pervasives.stdout
        in Enum.iter (fun x -> if opts.proc_echo then (Pervasives.output_char oc x; fl Pervasives.stdout )
                                                 else () ) (input_chars c)

    in let answ_wait opts t = 
        let fl = match opts.proc_filt with Some(i,o) -> (descr_of_in_channel i)::[] | _ -> []
        in match Thread.select [in_fd;] fl [] t with
                | (x::_,_,_) -> Event.sync( (Event.send ch_b EAnsw) ); dump_out opts (in_channel_of_descr x)
                | _          -> Event.sync( (Event.send ch_b ETimeout) )

    in let inp_wait opts () =
        match Thread.select [in_fd] [] [] 0.0001 with
        | (x::_,_,_) -> dump_out opts (in_channel_of_descr x)
        | _         -> ()

    in let spawn_filter p = Unix.open_process p

    in let rec process_answ opts = 
        let evt = Event.poll (Event.receive ch_a)
        in try
            let _ = 
                match evt with
                | Some(EWaitAnswer(t))  -> answ_wait opts t
                | Some(EAbortThread)    -> raise ThreadExit
                | Some(ESetEcho(false)) -> process_answ { opts with proc_echo = false }
                | Some(ESetEcho(true))  -> process_answ { opts with proc_echo = true  }
                | Some(EPrintString(s)) -> printf "%s" s; fl Pervasives.stdout
                | Some(ERunFilter(p))   -> process_answ { opts with proc_filt = Some(spawn_filter p)}
                | _                     -> inp_wait opts ()
            in process_answ opts
        with _ -> match opts.proc_filt with Some(p) -> let _ = Unix.close_process p in ()
                                          | None  -> ()

    in let send_char c = match c with
    | '\r' | '\n' | '\t' | ' ' -> output_char outp c; flush(); Thread.delay 0.0003
    | x                        -> output_char outp x;

    in let cmd_lexer = make_lexer ["wait_input";"wait";"timeofday";"bye";"echo"; "print";"run_filter"]

    in let wait f = Thread.delay f

    in let rec wait_input f =
        let t1 = Unix.gettimeofday ()
        in let _ = Event.sync ( Event.send ch_a (EWaitAnswer(f)) )
        in match Event.select [(Event.receive ch_b);] with
        | EAnsw   -> (); (*printf "Got  answ in %f\n" ( Unix.gettimeofday() -. t1)*)
        | _       -> (); (*printf "Got timeout in %f\n" ( Unix.gettimeofday() -. t1)*)

    in let set_echo echo = 
        if echo == 0 then Event.sync( Event.send ch_a (ESetEcho(false)) )
        else              Event.sync( Event.send ch_a (ESetEcho(true)) )

    in let out_string s = Event.sync( Event.send ch_a (EPrintString(s)) )

    in let run_filter path = Event.sync( Event.send ch_a (ERunFilter(path)) )

    in let terminate () = Event.sync( Event.send ch_a EAbortThread )

    in let rec cmd_parser = parser 
        | [< 'Kwd "wait_input"; 'Float f >]    -> wait_input f
        | [< 'Kwd "wait"; 'Float f >]          -> wait f
        | [< 'Kwd "timeofday"  >]              -> ()
        | [< 'Kwd "echo";  'Int v  >]          -> set_echo v 
        | [< 'Kwd "print"; 'String v  >]       -> out_string v 
        | [< 'Kwd "run_filter"; 'String v  >]  -> run_filter v
        | [< 'Kwd "bye" >]                     -> terminate (); raise Bye 
        | [<>]                                 -> ()

    in let exec_cmd cmd =
        cmd |> Stream.of_string |> cmd_lexer |> cmd_parser

    in let rec play ll = match ll with
        | '%' :: xs -> let cmd = List.takewhile (fun x -> x != '\n') xs
                       in let rest = List.drop ((List.length cmd) + 1) xs
                       in let _ = exec_cmd (String.implode cmd)
                       in play rest
        | x :: xs   -> send_char x ; play xs
        | []        -> ()

    in let default_opts = { proc_echo = true; proc_filt = None }
    in let t1 = Thread.create (fun () -> process_answ default_opts) ()
    in let _  = play (List.of_enum script)
    in let _  = Event.sync( Event.send ch_a (EAbortThread))
    in let _  = Thread.join t1
    in ()

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

