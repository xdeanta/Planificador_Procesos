all: despachador child
despachador: plan.c
	gcc plan.c -o despachador
child: child.c
	gcc child.c -o child
clean:
	rm -f despachador child
