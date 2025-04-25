MEMORY
{
	ram	(rwx) : ORIGIN = 0x00000000, LENGTH = 0x00010000
}

ENTRY(_start)

SECTIONS
{
	.text :
	{
		*(*.text*)	
	} > ram
}
