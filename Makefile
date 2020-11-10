all:	app

app:	app.c linkLayer.c
		gcc app.c linkLayer.c -o app 



	
