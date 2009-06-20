open Genlex
open Printf
open Unix
open Str
open ExtString
open ExtList
open Std

let (|>) f x = x f

exception Bye

type events = EWaitAnyChar of float | EWaitAnswer of float | ESuspend | ERelease

type opts  = {
               mutable opt_port     : string;
               mutable opt_baudrate : int;
               mutable opt_script   : string 
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
  in let ic = in_channel_of_descr fd
  in let oc = out_channel_of_descr fd
  in (ic, oc)

let run_script opts (inp,outp) script  =
    let flush () = Pervasives.flush outp
    in let in_fd = descr_of_in_channel inp

    in let answ_wait t = Thread.delay t

    in let any_wait () = 
        match Thread.select [in_fd;] [] [] (-1.0) with
        | (x::_,_,_) -> Enum.iter (fun x -> printf "%c" x; fl Pervasives.stdout) (input_chars (in_channel_of_descr x)) 
        | _          -> ()

    in let rec process_answ ch = 
        let _ = match Event.poll( Event.receive ch)  with 
        | Some(EWaitAnswer(t)) -> answ_wait t
        | _                      -> any_wait ()
        in  process_answ ch

    in let send_char c = match c with
    | '\r' | '\n' | '\t' | ' ' -> output_char outp c; flush(); Thread.delay 0.005 
    | x                        -> output_char outp x;

    in let cmd_lexer = make_lexer ["wait_input";"bye"]

    in let wait_input f ch = 
        let _ = Event.sync( Event.send ch (EWaitAnswer(f)) )
        in match Thread.select [in_fd;] [] [] f with
        | (x::_,_,_) -> Enum.iter (fun x -> printf "%c" x; fl Pervasives.stdout) (input_chars (in_channel_of_descr x))
        | _          -> ()

    in let rec cmd_parser ch = parser 
        | [< 'Kwd "wait_input"; 'Float f; >] -> wait_input f ch
        | [< 'Kwd "bye"; >]                  -> raise Bye 
        | [<>]                               -> ()

    in let exec_cmd cmd ch = 
        cmd |> Stream.of_string |> cmd_lexer |> cmd_parser ch

    in let rec play ll ch = match ll with
        | '%' :: xs -> let cmd = List.takewhile (fun x -> x != '\n') xs
                       in let rest = List.drop ((List.length cmd) + 1) xs
                       in let _ = exec_cmd (String.implode cmd) ch
                       in play rest ch
        | x :: xs   -> send_char x ; play xs ch
        | []        -> ()

    
    in let ch = Event.new_channel ()
    in let t1 = Thread.create process_answ ch 
    in let _  = play (List.of_enum script) ch
    in let _  = Thread.delay 1.5 
    in ()

let _ = 
    let opts = get_args ()
    in let _ = printf "Running %s %d %s\n\n" opts.opt_port opts.opt_baudrate opts.opt_script
    in try  run_script opts (port opts) (script opts)
       with Bye -> print_endline "Bye!"

