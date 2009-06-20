open Ti_txt
open Printf
open ExtList

let max_block = 256

let (|>) f x = x f

let dump_data d = 
    List.iteri (fun i x -> printf "%04X%s" x (if ((i+1) mod 8) == 0 then "\n" else " ") ) d

let rec pack l = match l with
    | a :: b :: xs  -> ((b lsl 8) lor a) :: pack xs
    | a :: []       -> failwith "Data alignment error"
    | []            -> []

let split_block { b_addr = a; b_data = d} =
    let rec spl bs b = 
        if List.length b.b_data <= max_block then bs @ [b]
        else let     taken = List.take max_block b.b_data
             in let  rest  = List.drop max_block b.b_data
             in  spl (bs @ [{ b with b_data = taken }]) { b_addr = b.b_addr + max_block*2; b_data = rest}
    in spl [] { b_addr = a; b_data = d}

let dump_block_f block = 
    printf "reset\n" ;
    printf "buf\n" ;
    List.iteri (fun i x -> printf "$%04X !w+ %s" x (if (i+1) mod 8 == 0 then "\n" else " ") ) block.b_data ;
    printf "\n$%04X !xfe\n" block.b_addr ; 
    printf "%%wait_for_input 2.0\n" ;
    printf "$%04X $%04X !xfwm\n" block.b_addr (List.length block.b_data) ;
    printf "%%wait_for_input 2.0\n\n"

let dump_header_f () = 
    printf "0 echo\n";
    printf "1 led\n";
    printf "aquire\n"

let dump_footer_f () =
    printf "\n0 led\n";
    printf "release \n" ;
    printf "%%wait_for_input 2.0\n\n"

let _ = 
  let lex = Lexing.from_channel (Pervasives.stdin)
  in let ti = Parser.toplevel Lexer.token lex
  in let p  = List.map (fun x -> { x with b_data = pack x.b_data }) ti
  in let pp = List.fold_left (fun acc a ->  acc @ split_block a ) [] p 
  in dump_header_f () ; pp |> List.iter dump_block_f; dump_footer_f () 
(*  in List.iter (fun {b_addr = addr; b_data = data} -> printf "@%04X\n" addr ; dump_data data  ) pp*)


