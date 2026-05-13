==================MCP=========================
This simulates a process scheduler, with 4 different parts.
Part 1 simply runs all the processes in parallel
Part 2 does the same, but it stops them all with SIGSTOP, before continuing them all with SIGCONT
Part 3 implements a round robin scheduler, with CPU time rotating between the processes
Part 4 does the same but prints some info from proc
To run, download the files, and compile them with make.
Usage: ./partx -f input_file
Input File should be in the style of input.txt
