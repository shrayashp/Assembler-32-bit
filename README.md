# Assembler-32-bit
The Assembler we implemented includes all the SIC/XE instructions and supports all four formats 1, 2, 3, 4, addressing modes and program relocation.
Working of the Assembler:
PASS 1
1. Assigns address to all statements in the program.
2. Saves the values(addresses) assigned to labels for use in PASS2.
3. Perform some processing of the assembler directives(This includes
processing that affects address assignment, such as determining the
length of data areas defined by BYTE, RESW, etc).
4. Write any errors identified in the error_input.txt file under PASS1
heading.
PASS 2:
1. Assemble instructions (translating operation codes and looking up
addresses).
2. Generate data values defined by BYTE, WORD, etc.
3. Perform processing of assembler directives not done during PASS1.
4. Write the object program in the object_input.txt and the assembly
listing.
5. Write any errors identified in the error_input.txt file under PASS2
heading.
