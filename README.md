# mysh
Unix Shell From Scratch in C

This was written after reading a Unix internals book (FreeBSD at the time) to learn how shells work at a low-level.

For a one or two sitting project, this code is surprisingly capable and could be educational.

Features:
* Piping of (at max) 10 processes together, each with a maximum of 10 command line params.
* Command parser is pretty much done, but not all symbols are implemented yet (output redirection, etc).  
* Debug mode for extending shell commands, checkparsing, etc.

This code worked fine with GCC on FreeBSD back in the day, but not sure about now. :)
