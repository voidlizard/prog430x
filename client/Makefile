INCL = -I +extlib
PACKAGES = -package "extlib unix str threads"
FILES = client430x.ml
OUT = client430x

all:
	ocamlfind ocamlc -thread -linkpkg $(INCL) $(PACKAGES) $(FILES) -o $(OUT)

