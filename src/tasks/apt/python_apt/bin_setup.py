# -*- coding: utf-8 -*-
import sys
from cx_Freeze import setup, Executable

# buildOptions = dict(  
	# compressed = True,  
	# includes = ["serial"],  
	# include_files = ["./"],  
	# base = 'Win32GUI',  
	# path = sys.path + ["modules"])

setup(
	name = "APT C131",
	version = "0.1",
	description = "Linktronsys",
	# options = dict(build_exe = buildOptions),
	executables = [Executable("apt.py",base="Win32GUI",icon = "py.ico",)])
