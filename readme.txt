arch principal:
1) the whole software is divided into 5 layers, main, lib_task, drivers, cpu
and lib_basic. In general, the main layer is fixed, lib_task and 
lib_basic is joined together as lib, which should be device independent.
each one of lib_task acts like a thread. lib_basic contains some
standard algorithm, such as string handle, memory allocation or compress/decompress and etc.
2) Each layer can and can only call the layers below it and the partners in the same layer.
For example, lib_basic is located at the bottom layer, it could be called by
every layer.

folders description:
/projects	ide project files
/scripts	script files
/src		source codes
/doc		documents

/src/include	header files
/src/main	main program
/src/lib	modules of lib_task and lib_basic
/src/drivers 	device drivers
/src/cpu	all cpu supported


/src/lib/basic	standard low level algorithms<lib_basic>
/src/lib/gnu	gcc toolchain specific files<lib_basic>
/src/lib/ewarm	iar toolchain specific files<lib_basic>
/src/lib/system	basic system maintain task<lib_task>
/src/lib/motor	motor control specific files<lib_task>
/src/lib/shell	command line user interface<lib_task>

