alloc: alloc.c
	gcc -o alloc alloc.c
prov-rep: prov-rep.c
	gcc -o prov-rep prov-rep.c
run: alloc
	    ./a.out
.PHONY: all run
clean:
	rm -f alloc
