MEMORY
{
	ram	(rwx) : ORIGIN = 0x00000000, LENGTH = 0x00010000
}

STARTUP(crt0.o)
ENTRY(_start)

SECTIONS
{
	.text :
	{
		*(*.text*)	
	} > ram
}
