all:	writenoncanical noncanonical

writenoncanical:	writenoncanonical.c utils.c
		gcc writenoncanonical.c utils.c -o writenoncanonical

noncanonical:	noncanonical.c utils.c
		gcc noncanonical.c utils.c -o noncanonical

	
