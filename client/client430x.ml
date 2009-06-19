
open Printf
open Unix
open Str
open ExtString
open Std

type opts  = {
               mutable opt_port     : string;
               mutable opt_baudrate : int;
               mutable opt_script   : string 
             }

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

    in let rec process_answ () = 
        let fd = descr_of_in_channel inp
        in let _  = match Thread.select [fd;] [] [] (-1.0) with
        | (x::_,_,_) -> Enum.iter (fun x -> printf "%c" x; Pervasives.flush Pervasives.stdout) (input_chars (in_channel_of_descr x)) 
        | _          -> ()
        in  process_answ ()

    in let send_char c = match c with
    | '\r' | '\n' | '\t' | ' ' -> output_char outp c; flush(); Thread.delay 0.005 
    | x                        -> output_char outp x;

    in let t1 = Thread.create process_answ ()
    in let _  = Enum.iter send_char script
    in let _  = Thread.delay 2.0
    in ()

let _ = 
    let opts = get_args ()
    in let _ = printf "Running %s %d %s\n\n" opts.opt_port opts.opt_baudrate opts.opt_script
    in run_script opts (port opts) (script opts)

