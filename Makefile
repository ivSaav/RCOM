all:	writenoncanical noncanonical utils

writenoncanical:	writenoncanonical.c
		gcc writenoncanonical.c utils.c -o writenoncanonical

noncanonical:	noncanonical.c
		gcc noncanonical.c utils.c -o noncanonical

	
