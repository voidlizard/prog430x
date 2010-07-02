open Ti_txt
open Printf
open ExtList
open ExtString
open Crc16

let max_block = 256

let flash2_bound = 0x10000

let (|>) f x = x f

exception Align_error of int

type erase_method = ERASE_MASS | ERASE_SGMT

type opts = {
    mutable erase   : erase_method ;
    mutable verbose : bool;
    mutable crc16   : bool
}

type fw_params = {
    data_size: int
}


let dump_data d = 
    List.iteri (fun i x -> printf "%04X%s" x (if ((i+1) mod 8) == 0 then "\n" else " ") ) d

let rec pack l = match l with
    | a :: b :: xs  -> ((b lsl 8) lor a) :: pack xs
    | a :: []       -> raise (Align_error(a))
    | []            -> []

let split_block { b_addr = a; b_data = d} =
    let rec spl bs b =
        let flash2_cross = if b.b_addr < flash2_bound && b.b_addr + (List.length b.b_data) >= flash2_bound 
                           then true
                           else false
        in let block_size = if not flash2_cross then max_block else (min ((flash2_bound - b.b_addr) / 2) max_block)
        in if not flash2_cross && List.length b.b_data <= block_size then bs @ [b]
           else let     taken = List.take block_size b.b_data
                in let  rest  = List.drop block_size b.b_data
                in  spl (bs @ [{ b with b_data = taken }]) { b_addr = b.b_addr + block_size*2; b_data = rest}
    in spl [] { b_addr = a; b_data = d}


let rec strings_of_data strings (data:int list) = 
    let fmt chars =
        List.map (fun x ->sprintf "\\%03d\\%03d" (x land 0xFF) ((x land 0xFFFF) lsr 8) ) chars |> String.join ""

    in match List.length data with
    | 0                 -> strings
    | x when x < 8      -> strings @ [fmt data]
    | x                 -> strings_of_data (strings @ [fmt (List.take 8 data)]) (List.drop 8 data)

let dump_block_f opts block = 

    let size = (List.length block.b_data)*2 in
    let crc16 = (Crc16.crc16_calc block.b_data) in

    printf "%d %d readbytes\n" 0 size ;

    List.iter (fun s -> printf "%%sendstr \"%s\"\n" s ) (strings_of_data [] block.b_data) ;
    
    printf "%%CRC16: %04X\n\n" crc16  ;
    

(*    List.iteri (fun i x -> printf "$%04X !w+ %s" x (if (i+1) mod 8 == 0 then "\n" else " ") ) block.b_data ;*)

    if opts.erase == ERASE_SGMT || block.b_addr >= flash2_bound
    then
        begin
        printf "%%read_timeout 1.0\n" ;
        printf "\n\n$%04X !xfe\n" block.b_addr ; 
        end
    else
        begin
        printf "\n" ;
        end
    ;

    if opts.verbose then printf "\n%%print \"bytes written: %d\\n\"\n" size;
    printf "$%04X $%04X\n" block.b_addr (List.length block.b_data) ;
    printf "%%read_timeout 0.5\n" ;
    printf "!xfwm\n" ; 
    printf "%%read_timeout 0.00001\n" ;

    if opts.crc16 then
    begin
        printf "%%read_timeout 0.01\n" ;
        printf "$00 bfill\n" ;
        printf "$%04X $%03X @xm\n" block.b_addr size ;
        printf "0 %d calcrc $%04x -\n" size crc16;
        printf "%%print \"CRC CHECK: \"\n";
        printf ".x\n" ;
        printf "%%print \"\\n\"\n";
        printf "%%read_timeout 0.00001\n" ;
    end ;

    printf "\n"

let dump_header_f opts fw = 
    if opts.verbose then printf "%%print  \"bytes total: %d\\n\"\n" fw.data_size;
    printf "0 echo\n";
    printf "1 led\n";
    printf "%%read_timeout 1.0\n" ;
    printf "aquire\n";
    printf "%%read_timeout 0.00001\n" ;
    if opts.erase == ERASE_MASS then 
        ( printf "%%read_timeout 1.0\n" ;
          printf "\n$5C00 !xfem\n" ;
          printf "%%read_timeout 0.00001\n\n" ;
        )

let dump_footer_f opts =
    printf "\n0 led\n";
    printf "release \n" ;
    printf "%%wait_input 1.0\n\n"

let _ = 
    let opts = { erase = ERASE_SGMT; verbose = true; crc16 = true }
    in let _ = Arg.parse [
                          ("--mass",    Arg.Unit(fun () -> opts.erase   <- ERASE_MASS ), "mass erase");
                          ("--quiet",   Arg.Unit(fun () -> opts.verbose <- false), "reduce verbosity");
                          ("--nocrc16", Arg.Unit(fun () -> opts.crc16   <- false), "do not calc crc16")
                          ] 
                          (fun x -> ()) "Usage:"
    in let lex = Lexing.from_channel (Pervasives.stdin)
    in let ti = Parser.toplevel Lexer.token lex
    in let len = List.fold_left (fun acc x -> acc + (List.length x.b_data)) 0 ti
    in let fw = { data_size = len }
    in let p  = List.map (fun x -> try { x with b_data = pack x.b_data } with Align_error(al) -> failwith (sprintf "Align error at %08x near %04x" x.b_addr al)) ti
    in let pp = List.fold_left (fun acc a ->  acc @ split_block a ) [] p 
    in dump_header_f opts fw ; pp |> List.iter (fun x -> dump_block_f opts x); dump_footer_f opts

