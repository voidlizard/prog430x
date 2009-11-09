open Ti_txt
open Printf
open ExtList

let max_block = 256

let flash2_bound = 0x10000

let (|>) f x = x f

exception Align_error of int

type erase_method = ERASE_MASS | ERASE_SGMT

type opts = {
    mutable erase : erase_method
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

let dump_block_f opts block = 
    printf "reset\n" ;
    printf "buf\n" ;
    List.iteri (fun i x -> printf "$%04X !w+ %s" x (if (i+1) mod 8 == 0 then "\n" else " ") ) block.b_data ;

    begin
    match opts.erase with 
    | ERASE_SGMT 
    | _ when block.b_addr >= flash2_bound ->
        printf "\n\n$%04X !xfe\n" block.b_addr ; 
        printf "%%wait_input 0.8\n\n"
    | _ -> printf "\n\n"
    end ;

    printf "$%04X $%04X !xfwm\n" block.b_addr (List.length block.b_data) ;
    printf "%%wait_input 0.5\n\n"

let dump_header_f opts = 
    printf "0 echo\n";
    printf "1 led\n";
    printf "aquire\n";
    printf "%%wait_input 0.2\n" ;
    if opts.erase == ERASE_MASS then printf "\n$5C00 !xfem\n%%wait 0.8\n\n"

let dump_footer_f opts =
    printf "\n0 led\n";
    printf "release \n" ;
    printf "%%wait_input 2.0\n\n"

let _ = 
    let opts = { erase = ERASE_SGMT }
    in let _ = Arg.parse [("--mass", Arg.Unit(fun () -> opts.erase <- ERASE_MASS ), "mass erase")] (fun x -> ()) "Usage:"
    in let lex = Lexing.from_channel (Pervasives.stdin)
    in let ti = Parser.toplevel Lexer.token lex
    in let p  = List.map (fun x -> try { x with b_data = pack x.b_data } with Align_error(al) -> failwith (sprintf "Align error at %08x near %04x" x.b_addr al)) ti
    in let pp = List.fold_left (fun acc a ->  acc @ split_block a ) [] p 
    in dump_header_f opts ; pp |> List.iter (fun x -> dump_block_f opts x); dump_footer_f opts

