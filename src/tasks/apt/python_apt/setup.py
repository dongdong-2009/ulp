# -*- coding: utf-8 -*-
from cx_Freeze import setup, Executable  
 
#includeFiles = [
#	( r"D:\lib\Python26\tcl\tcl8.5", "tcl"),
#	( r"D:\lib\Python26\tcl\tk8.5", "tk")
#]
 
setup(
		name = "hello",
		version = "0.1",
		description = "Sample cx_Freeze script",
		# options = {"build_exe": {"include_files": includeFiles,}},
		# options = {"build_exe": {"include_files": includeFiles,}},
		executables = [Executable("apt.py",base="Win32GUI")])