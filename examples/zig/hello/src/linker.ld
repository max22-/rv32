MEMORY
{
	ram	(rwx) : ORIGIN = 0x00000000, LENGTH = 64K
}

PROVIDE(__stack_top = ORIGIN(ram) + LENGTH(ram));

SECTIONS
{
	
	.text :
	{
		*(*.text*)	
	} > ram

}