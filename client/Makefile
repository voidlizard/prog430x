TARGET=client430x

OCAMLBUILD=ocamlbuild

.PHONY: install

default: native 

all: byte native

byte:
	$(OCAMLBUILD) $(TARGET).byte

native:
	$(OCAMLBUILD) $(TARGET).native


clean:
	$(OCAMLBUILD) -clean

install:
	cp  $(TARGET).native ../bin/$(TARGET)

