#!/usr/bin/python
#This file is used by bldc project to modify iar project file(*.ewp) according to the 'make xconfig' result
#miaofng@2010 initial version
#

from lxml import etree
from sys import *
from os import *

#the same path viewed by different app
top_dir = getcwd()
dir_prefix = "$PROJ_DIR$/../../"

def usage():
	print "usage:"
	print "iar clr test.ewp"
	print "iar cfg test.ewp cpu icf"
	print "iar inc test.ewp src/include/"
	print "iar add test.ewp src/lib/ iar/ motor/ shell/ sys/ ..."
	print "iar ldf test.ewp --image_file $PROJ_DIR$\oid_ps.bmp.bin,image_main_window,.text,4"

def path_win(path):
	ret = "";
	size = len(path);
	for i in range(size):
		if (path[i] != '/'):
			ret = ret + path[i];
		else:
			ret = ret + '\\';
	return ret;

def iar_clr():
	#print 'iar_init()'
	if(nr_para < 3):
		usage();
		quit();
	fname = argv[2];
	iar = etree.parse(fname);
	root = iar.getroot();
	#remove groups
	grps = root.xpath("/project/group");
	nr_grps = len(grps)
	for i in range(nr_grps):
		grp = grps[i];
		print 'Group', grp.xpath("name")[0].text, 'is removed!'
		root.remove(grp)
	#remove c included directories
	opts = root.xpath("//option/name[text()='CCIncludePath2']/..");
	nr_opts = len(opts);
	for i in range(nr_opts):
		opt = opts[i];
		stats = opt.xpath("state");
		nr_stats = len(stats);
		for j in range(nr_stats):
			stat = stats[j];
			print 'CC Include Path', stat.text, 'is removed!'
			opt.remove(stat)
	#remove asm included directories
	opts = root.xpath("//option/name[text()='AUserIncludes']/..");
	nr_opts = len(opts);
	for i in range(nr_opts):
		opt = opts[i];
		stats = opt.xpath("state");
		nr_stats = len(stats);
		for j in range(nr_stats):
			stat = stats[j];
			print 'Asm Include Path', stat.text, 'is removed!'
			opt.remove(stat)
	#remove link options
	opts = root.xpath("//option/name[text()='IlinkExtraOptions']/..");
	nr_opts = len(opts);
	for i in range(nr_opts):
		opt = opts[i];
		stats = opt.xpath("state");
		nr_stats = len(stats);
		for j in range(nr_stats):
			stat = stats[j];
			print 'LD_FLAGS -=', stat.text
			opt.remove(stat)
	#write to original file
	iar.write(fname)


#iar cfg stm32/lm3s
def iar_cfg():
	#print 'iar_cfg()'
	if(nr_para < 4):
		usage();
		quit();
	fname = argv[2];
	#config cpu
	cpu = argv[3];
	dname = path.join(dir_prefix, path.dirname(argv[3]));
	iar = etree.parse(fname);
	root = iar.getroot();
	#find chip select option
	states = root.xpath("//option/name[text()='OGChipSelectEditMenu']/../state");
	nr_states = len(states);
	for i in range(nr_states):
		state = states[i];
		print 'cfg cpu:', state.text, '->', cpu
		state.text = cpu;
	#config icf file
	if(nr_para > 4):
		icf = path.join("$PROJ_DIR$/", argv[4]);
		#find icf option
		states = root.xpath("//option/name[text()='IlinkIcfFile']/../state");
		nr_states = len(states);
		for i in range(nr_states):
			state = states[i];
			print 'cfg icf:', state.text, '->', icf
			state.text = icf;
	#write to original file
	iar.write(fname)

#iar inc test.ewp src/include/
def iar_include():
	#print 'iar_include()'
	if(nr_para < 4):
		usage();
		quit();
	fname = argv[2];
	dname = path.join(dir_prefix, path.dirname(argv[3]));
	iar = etree.parse(fname);
	root = iar.getroot();
	#find include dir option - c include
	opts = root.xpath("//option/name[text()='CCIncludePath2']/..");
	nr_opts = len(opts);
	for i in range(nr_opts):
		opt = opts[i];
		print 'CC Include Path', dname, 'is added!'
		state = etree.SubElement(opt, "state");
		state.text = path_win(dname);
	#find include dir option - asm include
	opts = root.xpath("//option/name[text()='AUserIncludes']/..");
	nr_opts = len(opts);
	for i in range(nr_opts):
		opt = opts[i];
		print 'Asm Include Path', dname, 'is added!'
		state = etree.SubElement(opt, "state");
		state.text = path_win(dname);
	#write to original file
	iar.write(fname)

#iar ldf test.ewp --image_file $PROJ_DIR$\oid_ps.bmp.bin,image_main_window,.text,4
def iar_ld_flag():
	#print 'iar_include()'
	if(nr_para < 4):
		usage();
		quit();
	fname = argv[2];
	option = argv[3];
	if(nr_para > 4):
		option = option + " " + argv[4];
	iar = etree.parse(fname);
	root = iar.getroot();
	opts = root.xpath("//option/name[text()='IlinkExtraOptions']/..");
	nr_opts = len(opts);
	for i in range(nr_opts):
		opt = opts[i];
		print 'LD_FLAGS +=', option
		state = etree.SubElement(opt, "state");
		state.text = path_win(option);
	#write to original file
	iar.write(fname)

#iar add test.ewp src/lib/ iar/ motor/ shell/ sys/ ...
def iar_add():
	#print 'iar_add()'
	if(nr_para < 4):
		usage();
		quit();
	#get paras
	fname = argv[2];
	gname = path.dirname(argv[3]);
	iar = etree.parse(fname);
	root = iar.getroot();
	#find the container group
	cname = path.dirname(gname);
	cpath = "";
	while(cname != ""):
		dname = path.basename(cname);
		cpath = '/group[name=\'' + dname + '\']' + cpath;
		cname = path.dirname(cname);
	cpath = '/project' + cpath;
	econs = root.xpath(cpath);
	if(len(econs) < 1):
		print 'xpath{', cpath, '} not found'
		quit();
	econ = econs[0];
	#print etree.tostring(econ)
	#find the new group
	dname = path.basename(gname);
	cpath = cpath + '/group[name=\'' + dname + '\']';
	egrps = root.xpath(cpath);
	#not found? create the new group
	if len(egrps) < 1:
		print "grp", gname+'/', "is created"
		egrp = etree.SubElement(econ, "group");
		enam = etree.SubElement(egrp, "name");
		enam.text = gname;
		#print etree.tostring(econ)
	else:
		egrp = egrps[0];
	#handle empty group situation
	if(nr_para < 5):
		#print 'Group', econ.xpath("name")[0].text, '\\', egrp.xpath("name")[0].text, 'is removed!'
		econ.remove(egrp);
		iar.write(fname);
		quit();
	#create sub-group/sub-files
	for sname in argv[4:nr_para]:
		#if(path.dirname(sname) != ""):
		if(path.basename(sname) == ""):
			#create a sub-group
			print "sub-grp", sname, "is created"
			esub = etree.SubElement(egrp, "group");
			enam = etree.SubElement(esub, "name");
			enam.text = path.dirname(sname);
		else:
			#convert file name from .o to .c/.s
			size = len(sname);
			sname_c = sname[0:(size - 1)] + "c";
			sname_s = sname[0:(size - 1)] + "s";
			if (path.exists(path.join(top_dir, gname, sname_c))):
				sname = sname_c;
			elif (path.exists(path.join(top_dir, gname, sname_s))):
				sname = sname_s;
			else:
				print "WARNING: ", path.join(top_dir, gname, sname), "is not found!!!"
			#create a sub-file
			print "sub-file", sname, "is created"
			esub = etree.SubElement(egrp, "file");
			enam = etree.SubElement(esub, "name");
			enam.text = path_win(path.join(dir_prefix, gname, sname));
	#write to original file
	iar.write(fname)

#main()
nr_para = len(argv)
if(nr_para < 2):
	print 'Invalid Command Parameters'
	usage()
	quit()

if (argv[1] == 'clr'):
	iar_clr()
elif (argv[1] == 'cfg'):
	iar_cfg()
elif (argv[1] == 'add'):
	iar_add()
elif (argv[1] == 'inc'):
	iar_include()
elif (argv[1] == 'ldf'):
	iar_ld_flag();





