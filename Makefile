all:	writenoncanical noncanonical

writenoncanical:	writenoncanonical.c
		gcc writenoncanonical.c -o writenoncanonical

noncanonical:	noncanonical.c
		gcc noncanonical.c -o noncanonical
