This is a tiny line editor which can easiely ported to edit config files on a MCU.

Commands:

	lined [file] d23	- deletes line 23 in [file]

	lined [file] i2 Hello World!	- inserts "Hello World" on line 2 in [file]

	lined [file] a Hello World again - appends "Hello World again" to [file], if [file] does not exist it is created

	lined [file] r2 Goodbye World!	- replaces line 2 in [file] with "Goodbye World!"

	lined [file] p	- prints the content of [file]
