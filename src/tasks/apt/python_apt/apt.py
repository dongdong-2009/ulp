#!/user/bin/env python
import ConfigParser
import sys
import os
import apt_serial
import gobject
import scan
import time
#import Apt_Waiting
try:
	import pygtk
	pygtk.require("2.0")
except:
	pass

try:
	import gtk
	import gtk.glade
except:
	sys.exit(1)

conf = {"loop1":"on","loop2":"on","loop3":"on","loop4":"on","loop5":"on","loop6":"on",\
		"loop7":"on","loop8":"on","loop9":"on","loop10":"on","loop11":"on","loop12":"on","loop11b":"on","loop12b":"on",\
		"ess1":"on","ess2":"on","ess3":"on","ess4":"on","ess5":"on","ess6":"on",\
		"led1":"on","led2":"on","led3":"on","led4":"on","led5":"on",\
		"led1_a":"active low","led4_a":"active low",\
		"sw1":"on","sw2":"on","sw3":"on","sw4":"on","sw5":"on","sw1_s":"state1","sw2_s":"state1","sw5_s":"state1"}

datalog0 = [["OEM Part Number","********","F1 87"],["VINaa","********","F1 90"],\
			["Hardware Number","********","F1 92"],["Hardware Version Number","********","F1 93"],\
			["Software Number","********","F1 94"],["Software Version Number","********","F1 95"],\
			["Programming Date","********","F1 99"],["Configuration Informationi","********","01 00"],\
			["Data Information Field Version","********","F1 82"],["Calibration Data Information","********","F1 9B"],\
			["ECUInstallationDate","********","F1 9D"],["Delphi End Model Part Number","********","FD 00"],\
			["SDM checksum information","********","FD 01"],["SDM manufacturing data","********","fd 02"],\
			["Manufacturing Information","********","FD 03"]]

datalog1 = [["Seat Belt Position Status","********","FD 10"],["PAD SBR Status","********","FD 11"],\
			["Warning Indicator Status","********","FD 12"],["Supply voltage Status","********","FD 13"],\
			["Ignition Cycle Counter Status","********","FD 14"],["Real Time Status_AD Data","********","FD 20"],\
			["Real Time status_MUX Data","********","FD 21"],["SDM Accelerometer AD Data","********","FD 22"],\
			["SDM ROS AD Data","********","FD 23"],["Deployment Loop Assignments 1","********","FD 30"],\
			["Deployment Loop Assignments 2","********","FD 31"],["Deployment Loop Resistance 1","********","FD 32"],\
			["Deployment Loop Resistance 2","********","FD 33"],["ALDL","********","FD 40"],\
			["SDM Capability Configuration Contents","********","FD 41"],["SDM Programmed Configuration Contents","********","FD 42"]]

datalog2 = [["ESS Manufacturing Data","********","FD 50"],["ESS Acceleration Data","********","FD 51"],\
			["ESS Assignments","********","FD 52"],["ESS Interface Status","********","FD 53"],\
			["Driver FIS Verfication Data","********","FD 60"],["Passenger FIS Verfication Data","********","FD 61"],\
			["Driver P_SIS Verification Data","********","FD 62"],["Passenger P_SIS Verification Data","********","FD 63"],\
			["Driver SIS Verification Data","********","FD 64"],["Vehicle Status Data Infor","********","FD 65"],\
			["ESS 7 Verification Data","********","FD 66"],["ESS 8 Verification Data","********","FD 67"],\
			["SDM Counter Status 1 ESS1","********","FD 70"],["SDM Counter Status 2 ESS2","********","FD 71"],\
			["SDM Counter Status 3 ESS3","********","FD 72"],["SDM Counter Status 4 ESS4","********","FD 73"],\
			["SDM Counter Status 5 ESS5","********","FD 74"],["SDM Counter Status 6 ESS6","********","FD 75"]]

datalog3 = [["Vehicle Status Data Information","********","FD 80"],["Event Data Recorder 1_1","********","FD 90"],\
			["Event Data Recorder 1_2","********","FD 91"],["Event Data Recorder 1_3","********","FD 92"],\
			["EDR FIS Acceleration Data_Event1","********","FD 93"],["EDR SIS Acceleration Data_Event1","********","FD 94"],\
			["Event Data Recorder 1_4","********","FD 95"],["Event Data Recorder 2_1","********","FD A0"],\
			["Event Data Recorder 2_2","********","FD A1"],["Event Data Recorder 2_3","********","FD A2"],\
			["EDR FIS AccelerationData_Event2","********","FD A3"],["EDR SIS Acceleration Data_Event2","********","FD A4"],\
			["Event Data Recorder 2_4","********","FD A5"]]

datalog4 = [["switch","********"],["loop","********"]]

eeprom = [["FC 7A2 10 0A 23 44 00 EE 00 00"],["FC 7A2 10 0A 23 44 00 EE 00 FC"],\
		  ["FC 7A2 10 0A 23 44 00 EE 01 F8"],["FC 7A2 10 0A 23 44 00 EE 02 F4"],\
		  ["FC 7A2 10 0A 23 44 00 EE 03 F0"],["FC 7A2 10 0A 23 44 00 EE 04 EC"],\
		  ["FC 7A2 10 0A 23 44 00 EE 05 E8"],["FC 7A2 10 0A 23 44 00 EE 06 E4"],\
		  ["FC 7A2 10 0A 23 44 00 EE 07 E0"],["FC 7A2 10 0A 23 44 00 EE 08 DC"],\
		  ["FC 7A2 10 0A 23 44 00 EE 09 D8"],["FC 7A2 10 0A 23 44 00 EE 0A D4"],\
		  ["FC 7A2 10 0A 23 44 00 EE 0B D0"],["FC 7A2 10 0A 23 44 00 EE 0C CC"],\
		  ["FC 7A2 10 0A 23 44 00 EE 0D C8"],["FC 7A2 10 0A 23 44 00 EE 0E C4"],\
		  ["40 7A2 10 0A 23 44 00 EE 0F C0"]]

dtc_infor = [""]

config_stream = ["apt","add"]
set_stream = ["apt","set"]
tmp_data = [0xff,0xef,0xff,0xff,0xff,0xff,0x75,0xff]
send_stream = [set_stream[0],set_stream[1],"c131","f","f","e","f",  "f","f","f","f",   "f","f","f","f",   "7","5","f","f"]  #apt + add + name + byte(0-7)
pwr_stream = "apt","pwr"
send_pwr_stream = [pwr_stream[0],pwr_stream[1],"off","led"]

class Glade_main(gtk.Window):
	def __init__(self):
		main_dir = "./"
		glade_file = os.path.join(main_dir, "apt_main.glade")
		self.config_file = os.path.join(main_dir, "apt_config.ini")
		self.config = ConfigParser.ConfigParser()#RawConfigParser
		#self.waiting = Apt_Waiting.PyApp()
		self.builder = gtk.Builder()
		self.builder.add_from_file(glade_file)
		self.windows = self.builder.get_object("MainWindow")        
		self.about_dialog = self.builder.get_object("about_dialog")
		self.help_about = self.builder.get_object("help_about")
		self.error_dialog = self.builder.get_object("error_dialog")
		self.ok_dialog = self.builder.get_object("ok_dialog")
		self.send_dialog = self.builder.get_object("dialog_send")
		self.filechooser_dialog = self.builder.get_object("filechooser_dialog")
		self.dialog_save = self.builder.get_object("dialog_save")
		self.messagedialog_input = self.builder.get_object("messagedialog_input")
		self.entry_messagedialog = self.builder.get_object("entry")
		self.widget_save = self.builder.get_object("widget_save")
		self.messagedialog = self.builder.get_object("messagedialog")
		self.menu_quit = self.builder.get_object("menuitem_quit")
		self.statusbar = self.builder.get_object("statusbar")

		self.scan=scan.scan()
		self.combobox_com=self.builder.get_object("combo_com")
		liststore_com = gtk.ListStore(str)
		cell_com = gtk.CellRendererText()
		self.combobox_com.pack_start(cell_com)
		self.combobox_com.add_attribute(cell_com, 'text', 0)
		for n,s in self.scan:
			print "(%d) %s" % (n,s)
			liststore_com.append([s])
			self.combobox_com.set_model(liststore_com)
		self.combobox_com.set_active(0)

		self.combobox_baud=self.builder.get_object("combo_baud")
		liststore_baud = gtk.ListStore(str)
		cell_baud = gtk.CellRendererText()
		self.combobox_baud.pack_start(cell_baud)
		self.combobox_baud.add_attribute(cell_baud, 'text', 0)
		liststore_baud.append(["2400"])
		liststore_baud.append(["4800"])
		liststore_baud.append(["9600"])
		liststore_baud.append(["19200"])
		liststore_baud.append(["115200"])
		self.combobox_baud.set_model(liststore_baud)
		self.combobox_baud.set_active(4)
		self.combobox_baud.set_sensitive(False)

		liststore_filetype=gtk.ListStore(str)
		cell_filetype = gtk.CellRendererText()

		self.combobox_led1 = self.builder.get_object("combo_led1")
		liststore_led1 = gtk.ListStore(str)
		cell_led1 = gtk.CellRendererText()
		self.combobox_led1.pack_start(cell_led1)
		self.combobox_led1.add_attribute(cell_led1, 'text', 0)
		liststore_led1.append(["active low"])
		liststore_led1.append(["active high"])
		self.combobox_led1.set_model(liststore_led1)
		self.combobox_led1.set_active(0)
		self.combobox_led1.connect("changed",self.led1combobox_change,None)

		self.combobox_led4 = self.builder.get_object("combo_led4")
		self.combobox_led4.pack_start(cell_led1)
		self.combobox_led4.add_attribute(cell_led1, 'text', 0)
		self.combobox_led4.set_model(liststore_led1)
		self.combobox_led4.set_active(0)
		self.combobox_led4.connect("changed",self.led4combobox_change,None)

		self.combobox_sw1 = self.builder.get_object("combo_sw1")
		liststore_sw1 = gtk.ListStore(str)
		cell_sw1 = gtk.CellRendererText()
		self.combobox_sw1.pack_start(cell_sw1)
		self.combobox_sw1.add_attribute(cell_sw1, 'text', 0)
		liststore_sw1.append(["State1"])
		liststore_sw1.append(["State2"])
		self.combobox_sw1.set_model(liststore_sw1)
		self.combobox_sw1.set_active(0)
		self.combobox_sw1.connect("changed",self.sw1combobox_change,None)

		self.combobox_sw2 = self.builder.get_object("combo_sw2")
		self.combobox_sw2.pack_start(cell_sw1)
		self.combobox_sw2.add_attribute(cell_sw1, 'text', 0)
		self.combobox_sw2.set_model(liststore_sw1)
		self.combobox_sw2.set_active(0)
		self.combobox_sw2.connect("changed",self.sw2combobox_change,None)

		self.combobox_sw3 = self.builder.get_object("combo_sw3")
		liststore_sw3 = gtk.ListStore(str)
		cell_sw3 = gtk.CellRendererText()
		self.combobox_sw3.pack_start(cell_sw3)
		self.combobox_sw3.add_attribute(cell_sw3, 'text', 0)
		self.combobox_sw3.set_wrap_width(1)
		liststore_sw3.append(["Short"])
		liststore_sw3.append(["Open"])
		self.combobox_sw3.set_model(liststore_sw3)
		self.combobox_sw3.set_active(0)
		self.combobox_sw3.set_sensitive(False)


		self.combobox_sw4 = self.builder.get_object("combo_sw4")
		self.combobox_sw4.pack_start(cell_sw3)
		self.combobox_sw4.add_attribute(cell_sw3, 'text', 0)
		self.combobox_sw4.set_model(liststore_sw3)
		self.combobox_sw4.set_wrap_width(1)
		self.combobox_sw4.set_active(0)
		self.combobox_sw4.set_sensitive(False)


		self.combobox_sw5 = self.builder.get_object("combo_sw5")
		self.combobox_sw5.pack_start(cell_sw1)
		self.combobox_sw5.add_attribute(cell_sw1, 'text', 0)
		self.combobox_sw5.set_model(liststore_sw1)
		self.combobox_sw5.set_active(0)
		self.combobox_sw5.connect("changed",self.sw5combobox_change,None)

		self.label=self.builder.get_object("label")
		self.label.set_label("COM is "+'''\n Baud is''')
		self.connect_but = self.builder.get_object("connect_but")
		self.disconnect_but = self.builder.get_object("disconnect_but")
		self.button_quit = self.builder.get_object("button_quit")
		self.button_sdmpwr = self.builder.get_object("button_sdmpwr")
		self.button_save = self.builder.get_object("button_save")
		self.button_get_cfg = self.builder.get_object("button_get_cfg")
		self.button_filechooser = self.builder.get_object("button_filechooser")
		filt = gtk.FileFilter()
		filt.set_name("Configure file(*.ini)")
		filt.add_pattern("*.ini")
		self.button_filechooser.add_filter(filt)
		self.filechooser_dialog.add_filter(filt)
		self.widget_save.add_filter(filt)
		filt2 = gtk.FileFilter()
		filt2.set_name("All file(*.*)")
		filt2.add_pattern("*.*")
		self.widget_save.add_filter(filt2)

		self.button_filechooser.connect("file-set",self.filechooser_clicked)

		self.COM = apt_serial.ComThread(0,9600)
		self.image_lamp1 = self.builder.get_object("image_lamp1")
		self.image_lamp2 = self.builder.get_object("image_lamp2")
		self.image_lamp3 = self.builder.get_object("image_lamp3")
		self.image_lamp4 = self.builder.get_object("image_lamp4")
		self.image_lamp5 = self.builder.get_object("image_lamp5")
		self.image_status = self.builder.get_object("image_status")
		self.image_sdm = self.builder.get_object("image_sdm")
		self.image_sq1 = self.builder.get_object("image_sq1")
		self.image_sq2 = self.builder.get_object("image_sq2")
		self.image_sq3 = self.builder.get_object("image_sq3")
		self.image_sq4 = self.builder.get_object("image_sq4")
		self.image_sq5 = self.builder.get_object("image_sq5")
		self.image_sq6 = self.builder.get_object("image_sq6")
		self.image_sq7 = self.builder.get_object("image_sq7")
		self.image_sq8 = self.builder.get_object("image_sq8")
		self.image_sq9 = self.builder.get_object("image_sq9")
		self.image_sq10 = self.builder.get_object("image_sq10")
		self.image_sq11 = self.builder.get_object("image_sq11")
		self.image_sq12 = self.builder.get_object("image_sq12")
		self.image_sq11b = self.builder.get_object("image_sq11b")
		self.image_sq12b = self.builder.get_object("image_sq12b")
		self.image_ess1 = self.builder.get_object("image_ess1")
		self.image_ess2 = self.builder.get_object("image_ess2")
		self.image_ess3 = self.builder.get_object("image_ess3")
		self.image_ess4 = self.builder.get_object("image_ess4")
		self.image_ess5 = self.builder.get_object("image_ess5")
		self.image_ess6 = self.builder.get_object("image_ess6")
		self.image_sw1 = self.builder.get_object("image_sw1")
		self.image_sw2 = self.builder.get_object("image_sw2")
		self.image_sw3 = self.builder.get_object("image_sw3")
		self.image_sw4 = self.builder.get_object("image_sw4")
		self.image_sw5 = self.builder.get_object("image_sw5")

		self.button_lamp1 = self.builder.get_object("button_lamp1")
		self.button_lamp2 = self.builder.get_object("button_lamp2")
		self.button_lamp3 = self.builder.get_object("button_lamp3")
		self.button_lamp4 = self.builder.get_object("button_lamp4")
		self.button_lamp5 = self.builder.get_object("button_lamp5")
		self.button_sq1 = self.builder.get_object("button_sq1")
		self.button_sq2 = self.builder.get_object("button_sq2")
		self.button_sq3 = self.builder.get_object("button_sq3")
		self.button_sq4 = self.builder.get_object("button_sq4")
		self.button_sq5 = self.builder.get_object("button_sq5")
		self.button_sq6 = self.builder.get_object("button_sq6")
		self.button_sq7 = self.builder.get_object("button_sq7")
		self.button_sq8 = self.builder.get_object("button_sq8")
		self.button_sq9 = self.builder.get_object("button_sq9")
		self.button_sq10 = self.builder.get_object("button_sq10")
		self.button_sq11 = self.builder.get_object("button_sq11")
		self.button_sq12 = self.builder.get_object("button_sq12")
		self.button_sq11b = self.builder.get_object("button_sq11b")
		self.button_sq12b = self.builder.get_object("button_sq12b")
		self.button_ess1 = self.builder.get_object("button_ess1")
		self.button_ess2 = self.builder.get_object("button_ess2")
		self.button_ess3 = self.builder.get_object("button_ess3")
		self.button_ess4 = self.builder.get_object("button_ess4")
		self.button_ess5 = self.builder.get_object("button_ess5")
		self.button_ess6 = self.builder.get_object("button_ess6")
		self.button_sw1 = self.builder.get_object("button_sw1")
		self.button_sw2 = self.builder.get_object("button_sw2")
		self.button_sw3 = self.builder.get_object("button_sw3")
		self.button_sw4 = self.builder.get_object("button_sw4")
		self.button_sw5 = self.builder.get_object("button_sw5")
		self.button_send = self.builder.get_object("button_send")
		self.button_clr_dtc = self.builder.get_object("button_clr_dtc")
		self.button_stop_can = self.builder.get_object("button_stop_can")
		self.button_get_apt_diag = self.builder.get_object("button_get_apt_diag")
		self.button_get_sdm_diag = self.builder.get_object("button_get_sdm_diag")
		self.button_get_eeprom = self.builder.get_object("button_get_eeprom")
		self.button_save_diag = self.builder.get_object("button_save_diag")

		self.treeview = self.builder.get_object("treeview")
		self.store = self.create_model()
		self.treeview.set_model(self.store)
		self.create_columns(self.treeview)
		self.treeview.connect("row_activated",self.on_activated)

		self.led_off = gtk.gdk.pixbuf_new_from_file("led_off.png")
		self.led_on = gtk.gdk.pixbuf_new_from_file("led_on.png")
		self.connect_on = gtk.gdk.pixbuf_new_from_file("connect_on.png")
		self.connect_off = gtk.gdk.pixbuf_new_from_file("connect_off.png")   
		self.sdm_on = gtk.gdk.pixbuf_new_from_file("sdm_on.png")
		self.sdm_off = gtk.gdk.pixbuf_new_from_file("sdm_off.png")
		self.switch_on = gtk.gdk.pixbuf_new_from_file("switch_on.png")
		self.switch_off = gtk.gdk.pixbuf_new_from_file("switch_off.png")

		self.windows.connect("destroy",gtk.main_quit)
		self.menu_quit.connect("activate",gtk.main_quit)
		self.help_about.connect("activate",self.on_about_dialog)
		self.combobox_baud.connect("changed",self.on_change)
		self.connect_but.connect("clicked",self.connect_clicked)
		self.disconnect_but.connect("clicked",self.disconnect_clicked)
		self.button_quit.connect("clicked",gtk.main_quit)
		self.button_lamp1.connect("clicked",self.buttonlamp1_clicked,None)
		self.button_lamp2.connect("clicked",self.buttonlamp2_clicked,None)
		self.button_lamp3.connect("clicked",self.buttonlamp3_clicked,None)
		self.button_lamp4.connect("clicked",self.buttonlamp4_clicked,None)
		self.button_lamp5.connect("clicked",self.buttonlamp5_clicked,None)
		self.button_sdmpwr.connect("clicked",self.buttonsdmpwr_clicked)
		self.button_sq1.connect("clicked",self.buttonsq1_clicked,None)
		self.button_sq2.connect("clicked",self.buttonsq2_clicked,None)
		self.button_sq3.connect("clicked",self.buttonsq3_clicked,None)
		self.button_sq4.connect("clicked",self.buttonsq4_clicked,None)
		self.button_sq5.connect("clicked",self.buttonsq5_clicked,None)
		self.button_sq6.connect("clicked",self.buttonsq6_clicked,None)
		self.button_sq7.connect("clicked",self.buttonsq7_clicked,None)
		self.button_sq8.connect("clicked",self.buttonsq8_clicked,None)
		self.button_sq9.connect("clicked",self.buttonsq9_clicked,None)
		self.button_sq10.connect("clicked",self.buttonsq10_clicked,None)
		self.button_sq11.connect("clicked",self.buttonsq11_clicked,None)
		self.button_sq12.connect("clicked",self.buttonsq12_clicked,None)
		self.button_sq11b.connect("clicked",self.buttonsq11b_clicked,None)
		self.button_sq12b.connect("clicked",self.buttonsq12b_clicked,None)
		self.button_ess1.connect("clicked",self.buttoness1_clicked,None)
		self.button_ess2.connect("clicked",self.buttoness2_clicked,None)
		self.button_ess3.connect("clicked",self.buttoness3_clicked,None)
		self.button_ess4.connect("clicked",self.buttoness4_clicked,None)
		self.button_ess5.connect("clicked",self.buttoness5_clicked,None)
		self.button_ess6.connect("clicked",self.buttoness6_clicked,None)
		self.button_sw1.connect("clicked",self.buttonsw1_clicked,None)
		self.button_sw2.connect("clicked",self.buttonsw2_clicked,None)
		self.button_sw3.connect("clicked",self.buttonsw3_clicked,None)
		self.button_sw4.connect("clicked",self.buttonsw4_clicked,None)
		self.button_sw5.connect("clicked",self.buttonsw5_clicked,None)
		self.button_send.connect("clicked",self.buttonsend_clicked)
		self.button_save.connect("clicked",self.buttonsave_clicked,"configure")
		self.button_get_cfg.connect("clicked",self.buttongetcfg_clicked)
		self.button_get_apt_diag.connect("clicked",self.buttonget_apt_diag_clicked)
		self.button_get_sdm_diag.connect("clicked",self.buttonget_sdm_diag_clicked)
		self.button_get_eeprom.connect("clicked",self.button_get_eeprom_clicked)
		self.button_clr_dtc.connect("clicked",self.buttonclr_dtc_clicked,"Are you sure clear DTC? ","Clear DTC")
		self.button_stop_can.connect("clicked",self.buttonstop_can_clicked)
		self.button_save_diag.connect("clicked",self.buttonsave_clicked,"diag")
		self.set_widget(False)
		self.set_configure(conf)
		gtk.Window.maximize(self.windows)
		self.combobox_led1.set_active(1)
		self.windows.show()

	def remove_all_section(self):
		print self.config.sections()
		for act in self.config.sections():
			self.config.remove_section(act)
		self.config.sections()
		return

	def create_model(self): 
		store = gtk.TreeStore(str, str) 
		it1 = store.append(None,["Item A",""])
		for act in datalog0:
			string = act[0]
			string = "self."+ string.replace(" ","")
			exec(string + " = store.append(it1,[act[0], act[1]])")
		it2 = store.append(None,["Item B",""])
		for act in datalog1:
			string = act[0]
			string = "self."+ string.replace(" ","")
			exec(string + " = store.append(it2,[act[0], act[1]])")
		it3 = store.append(None,["Item C",""])
		for act in datalog2:
			string = act[0]
			string = "self."+ string.replace(" ","")
			exec(string + " = store.append(it3,[act[0], act[1]])")

		it4 = store.append(None,["Item D",""])
		for act in datalog3:
			string = act[0]
			string = "self."+ string.replace(" ","")
			exec(string + " = store.append(it4,[act[0], act[1]])")

		it5 = store.append(None,["Item Others",""])
		self.dtc = store.append(it5,["DTC","********"])

		it6 = store.append(None,["SELF DIAGNOSIS",""])
		self.diagnosis = store.append(it6,["********","********"])
		return store

	def create_columns(self, treeView): 
		rendererText = gtk.CellRendererText() 
		column = gtk.TreeViewColumn("Item", rendererText, text=0) 
		column.set_sort_column_id(0)     
		treeView.append_column(column) 

		rendererText = gtk.CellRendererText() 
		column = gtk.TreeViewColumn("Value", rendererText, text=1) 
		column.set_sort_column_id(1) 
		treeView.append_column(column)
		
	def on_activated(self,widget,row,col):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
		elif self.button_sdmpwr.get_label() == "ON":
			self.display_error(self.error_dialog,"Please Turn On SDM Power!")
		else:
			if len(row) == 1 or row[0] == 5:
				return
			test_number = 3	#times to try get the sdm diag data
			commd ="datalog"+str(row[0])+"[row[1]][2]"
			if row[0] == 4:
                                while True:
                                        self.COM.send("apt read dtc")
                                        self.COM.read()
                                        self.COM.send("\r")
                                        string = ''
                                        time.sleep(0.3)
                                        str_temp = self.COM.read()
                                        while str_temp != '':
                                                string += str_temp
                                                str_temp = self.COM.read()
                                        spit_list = string.splitlines()
					if spit_list[2] != "##OK##":
                                                test_number -= 1
                                        else:
                                                dtc_infor[0] = ''
                                                for act in spit_list:
                                                        #print act
                                                        spit__list = act.split()
                                                        length = len(spit__list)
                                                        if(length > 2):
                                                                dtc_infor[0] += act
                                                                dtc_infor[0] += "\n"
                                                        else:
                                                                continue
                                                self.store.set(self.dtc ,1,dtc_infor[0])
                                                return

                                        if test_number > 0:
                                                continue
                                        else:
                                                return 
                        while True:
                                self.COM.send("apt req 7A2 03 22 " + eval(commd) + " 00 00 00 00")
                                self.COM.read()
                                self.COM.send("\r")
                                time.sleep(0.3)
                                str_temp = self.COM.read()  
                                string = ''
                                while str_temp != '':
                                        string += str_temp
                                        str_temp = self.COM.read()
                                spit_list = string.splitlines()
                                str_temp = ''
                                for act in spit_list:
                                        print act
                                        spit__list = act.split()
                                        length = len(spit__list)
                                        if(length > 3 and spit__list[2] == "7C2"):
                                                str_temp += act
                                                str_temp += "\n"
                                        else:
                                                continue
                                if str_temp == '' and test_number > 0:
                                        test_number -= 1
                                        continue
                                temp_str = "datalog"+str(row[0])+"[row[1]][1]" + " = " + '''"""'''+str_temp + '''"""'''
                                exec(temp_str)
                                temp_str = "temp = datalog"+str(row[0])+'''[row[1]][0].replace(" ","")'''
                                exec(temp_str)
                                temp = "self."+ temp
                                temp_str ="self.store.set("+ temp +",1," + "datalog" + str(row[0])+"[row[1]][1])" 
                                exec(temp_str)
                                return

	def buttonget_sdm_diag_clicked(self,widget):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
		elif self.button_sdmpwr.get_label() == "ON":
			self.display_error(self.error_dialog,"Please Turn On SDM Power!")
		else:
			for n in range(5):
				str_temp = "datalog" + str(n)
				for m in range(len(eval(str_temp))):
					row = [n,m]
					self.on_activated(self.treeview,row,None)
			self.display_ok(self.ok_dialog,"Get SDM Diag Over!")

	def buttonget_apt_diag_clicked(self,widget):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
		else:
			for n,value in enumerate(datalog4):
				self.COM.send("apt diag "+ value[0])
				self.COM.read()
				self.COM.send("\r")
				str_temp = self.COM.read()
				string = ''
				while str_temp != '':
					string += str_temp
					str_temp = self.COM.read()
				spit_list = string.splitlines()
				string = ''
				for act in spit_list:
					spit__list = act.split()
					length = len(spit__list)
					if(length > 2):
						string += act
						string += "\n"
					else:
						continue
				datalog4[n][1]= string
				self.store.set(self.diagnosis ,n,string)
			self.display_ok(self.ok_dialog,"Get APT Diag Over!")

	def button_get_eeprom_clicked(self,widget):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
		elif self.button_sdmpwr.get_label() == "ON":
			self.display_error(self.error_dialog,"Please Turn On SDM Power!")
		else:
			response = self.dialog_save.run()
			self.dialog_save.hide()
			if response == gtk.RESPONSE_ACCEPT:
				config_file = self.widget_save.get_filename()
			else:
				return
			f=open(config_file,'w')
			f.write("###  EEPROM DATA LOG  ###\n")
			f.write(time.strftime('%Y-%m-%d  %H:%M:%S',time.localtime(time.time()))+"\n\n")
			for n in eeprom:
				f.write("Request:\n")
				f.write("ID = "+ n[0][3:] +"\n")
				f.write("ID = 7A2 21 00 00 00 "+ n[0][0:3] +"00 00 00\n")
				f.write("ID = 7A2 30 00 00 00 00 00 00 00\n")

				test_number = 3	#times to try get the eeprom data
				file_string = 'Response:\n'
				while True:
					self.COM.send("apt eeprom " + n[0])
					print "apt eeprom " + n[0]
					self.COM.read()
					self.COM.send("\r")
					string = ''
					time.sleep(0.3)
					str_temp = self.COM.read()
					while str_temp != '':
						string += str_temp
						str_temp = self.COM.read()
					spit_list = string.splitlines()
					if len(spit_list) < 5:
                                                break
					if spit_list[2] == "##ERROR##" :
                                                test_number -= 1
                                        elif spit_list[2] == "##OK##":
                                                for act in spit_list:
                                                        #print act
                                                        spit__list = act.split()
                                                        if len(spit__list) > 0 and spit__list[0]== "ID":
                                                        #length = len(spit__list)
                                                                file_string += act
                                                                file_string += "\n"
                                                        #f.write("\n")
                                                break
                                        else:
                                                break

                                        if test_number > 0:
                                                continue
                                        else:
                                                break 
           
				f.write(file_string)
				f.write("\n\n")
			print "Reading EEPROM OK!"
			self.display_ok(self.ok_dialog,"Save EEPROM Over!")
			f.close

	def buttonclr_dtc_clicked(self,widget,text,title):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
		elif self.button_sdmpwr.get_label() == "ON":
			self.display_error(self.error_dialog,"Please Turn On SDM Power!")
		else:
			self.messagedialog.set_title(title)
			self.messagedialog.set_markup(text)
			response = self.messagedialog.run()
			self.messagedialog.hide()
			if response == gtk.RESPONSE_YES:
				self.COM.send("apt clr dtc")
				self.COM.read()
				self.COM.send("\r")
				string = self.COM.read()
				result = string.find("##OK##")
				print result
				if result != -1 :
					self.display_ok(self.ok_dialog,"Clear Successful!")
				else :
					self.display_error(self.error_dialog,"Clear DTC failed !","Clear Failed")
			elif response == gtk.RESPONSE_NO:
				pass

	def buttonstop_can_clicked(self,widget):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
		else:
			if widget.get_label() == "Stop Vehicle CAN":
				self.COM.send("apt can off")
                                self.COM.read()
                                self.COM.send("\r")
                                string = self.COM.read()
                                result = string.find("##OK##")
				widget.set_label("Sart Vehicle CAN")
                                print string
			else:
				self.COM.send("apt can on")
                                self.COM.read()
                                self.COM.send("\r")
                                string = self.COM.read()
                                result = string.find("##OK##")
				widget.set_label("Stop Vehicle CAN")
                                print string
			
			if result != -1 :
				self.display_ok(self.ok_dialog,"STOP Vehicle CAN Message Successful!")
			else :
				self.display_error(self.error_dialog,"STOP Vehicle CAN Message  Failed!","STOP Vehicle CAN Message")

	def on_about_dialog (self, widget):
		self.about_dialog.run()
		self.about_dialog.hide()

	def display_error(self, widget,text,title = "Error",scope = None):
		if scope == None:
			self.error_dialog.set_title(title)
			self.error_dialog.set_markup(text)
			self.error_dialog.run()
			self.error_dialog.hide()
		elif scope == "serial":
			self.statusbar.push(0,"Serial port is not connected!")

	def display_ok(self, widget,text,title = "OK",scope = None):
		self.ok_dialog.set_title(title)
		self.ok_dialog.set_markup(text)
		self.ok_dialog.run()
		self.ok_dialog.hide()

	def load_conf(self,file_name,conf):
		self.remove_all_section()
		self.config.read(file_name)     #read configure file
		try:
			conf['loop1'] = self.config.get("loop", "loop1")
			conf['loop2'] = self.config.get("loop", "loop2")
			conf['loop3'] = self.config.get("loop", "loop3")
			conf['loop4'] = self.config.get("loop", "loop4")
			conf['loop5'] = self.config.get("loop", "loop5")
			conf['loop6'] = self.config.get("loop", "loop6")
			conf['loop7'] = self.config.get("loop", "loop7")
			conf['loop8'] = self.config.get("loop", "loop8")
			conf['loop9'] = self.config.get("loop", "loop9")
			conf['loop10'] = self.config.get("loop", "loop10")
			conf['loop11'] = self.config.get("loop", "loop11")
			conf['loop12'] = self.config.get("loop", "loop12")
			conf['loop11b'] = self.config.get("loop", "loop11b")
			conf['loop12b'] = self.config.get("loop", "loop12b")
		except:
			self.display_error(self.error_dialog,"Load 'LOOP' config fail!","Load Failed")
			return -1
		for n in range(8):
			if conf['loop'+str(n+1)] == "on":
				tmp_data[0] = tmp_data[0] | (0x01 << n)
			elif conf['loop'+str(n+1)] == "off":
				tmp_data[0] = tmp_data[0] & (~(0x01 << n))
			else :
				self.display_error(self.error_dialog,"Load 'LOOP"+str(n+1)+"' config fail!","Load Failed")
				return -1
		print "%x" %(tmp_data[0])
		for n in range(4):
			if conf['loop'+str(n+9)] == "on":
				tmp_data[1] = tmp_data[1] | (0x01 << n)
			elif conf['loop'+str(n+9)] == "off":
				tmp_data[1] = tmp_data[1] & (~(0x01 << n))
			else :
				self.display_error(self.error_dialog,"Load 'LOOP"+str(n+1)+"' config fail!","Load Failed")
				return -1
		i = 11
		for n in (5,6):
			if conf['loop'+str(i)+'b'] == "on":
				tmp_data[1] = tmp_data[1] | (0x01 << n)
			elif conf['loop'+str(i)+'b'] == "off":
				tmp_data[1] = tmp_data[1] & (~(0x01 << n))
			else :
				self.display_error(self.error_dialog,"Load 'LOOP"+str(i)+"b' config fail!","Load Failed")
				return -1
			i = i+1
		print "%x" %(tmp_data[1])
		try:
			conf['ess1'] = self.config.get("ess", "ess1")
			conf['ess2'] = self.config.get("ess", "ess2")
			conf['ess3'] = self.config.get("ess", "ess3")
			conf['ess4'] = self.config.get("ess", "ess4")
			conf['ess5'] = self.config.get("ess", "ess5")
			conf['ess6'] = self.config.get("ess", "ess6")
		except:
			self.display_error(self.error_dialog,"Load 'ESS' config fail!","Load Failed")
			return -1
		for n in range(6):
			if conf['ess'+str(n+1)] == "on":
				tmp_data[2] = tmp_data[2] | (0x01 << n)
			elif conf['ess'+str(n+1)] == "off":
				tmp_data[2] = tmp_data[2] & (~(0x01 << n))
			else :
				self.display_error(self.error_dialog,"Load 'ESS"+str(n+1)+"' config fail!","Load Failed")
				return -1
		print "%x" %(tmp_data[2])

		try:
			conf['led1'] = self.config.get("led", "led1")
			conf['led2'] = self.config.get("led", "led2")
			conf['led3'] = self.config.get("led", "led3")
			conf['led4'] = self.config.get("led", "led4")
			conf['led5'] = self.config.get("led", "led5")
			conf['led1_a'] = self.config.get("led", "led1_a")
			conf['led4_a'] = self.config.get("led", "led4_a")
		except:
			self.display_error(self.error_dialog,"Load 'LED' config fail!","Load Failed")
			return -1
		i=0
		for n in (1,2,3,5,6):
			i = i+1
			if conf['led'+str(i)] == "on":
				tmp_data[4] = tmp_data[4] | (0x01 << n)
			elif conf['led'+str(i)] == "off":
				tmp_data[4] = tmp_data[4] & (~(0x01 << n))
			else :
				self.display_error(self.error_dialog,"Load 'LED"+str(i)+"' config fail!","Load Failed")
				return -1
		print "%x" %(tmp_data[4])
		i=1
		for n in (0,4):
			if conf['led'+str(i)+'_a'] == "active low":
				tmp_data[4] = tmp_data[4] & (~(0x01 << n))
			elif conf['led'+str(i)+'_a'] == "active high":
				tmp_data[4] = tmp_data[4] | (0x01 << n)
			else :
				self.display_error(self.error_dialog,"Load 'LED"+str(i)+"'_a config fail!","Load Failed")
				return -1
			i=i+3
		print "%x" %(tmp_data[4])
		try:
			conf['sw1'] = self.config.get("sw", "sw1")
			conf['sw2'] = self.config.get("sw", "sw2")
			conf['sw3'] = self.config.get("sw", "sw3")
			conf['sw4'] = self.config.get("sw", "sw4")
			conf['sw5'] = self.config.get("sw", "sw5")
			conf['sw1_s'] = self.config.get("sw","sw1_s")
			conf['sw2_s'] = self.config.get("sw","sw2_s")
			conf['sw5_s'] = self.config.get("sw","sw5_s")
		except:
			self.display_error(self.error_dialog,"Load 'SW' config fail!","Load Failed")
			return -1
		i=0
		for n in (0,2,4,5,6):
			i = i+1
			if conf['sw'+str(i)] == "on":
				tmp_data[6] = tmp_data[6] | (0x01 << n)
			elif conf['sw'+str(i)] == "off":
				tmp_data[6] = tmp_data[6] & (~(0x01 << n))
			else :
				self.display_error(self.error_dialog,"Load 'SW"+str(i)+"' config fail!","Load Failed")
				return -1
		print "%x" %(tmp_data[6])
		i = (1,2)
		n = (1,3)
		for rag in range(2):                                    #attention 
                        if conf['sw'+str(i[rag])+'_s'] == "state1":
				tmp_data[6] = tmp_data[6] | (0x01 << n[rag])
			elif conf['sw'+str(i[rag])+'_s'] == "state2":
				tmp_data[6] = tmp_data[6] & (~(0x01 << n[rag]))
			else :
				self.display_error(self.error_dialog,"Load 'SW"+str(i)+"' config fail!","Load Failed")
				return -1
		if conf['sw5_s'] == "state1":
			tmp_data[6] = tmp_data[6] & (~(0x01 << 7))
                elif conf['sw5_s'] == "state2":
                        tmp_data[6] = tmp_data[6] | (0x01 << 7)
		else :
			self.display_error(self.error_dialog,"Load 'SW"+str(i)+"' config fail!","Load Failed")
			return -1
		print conf
		return 0

	'''def display_filechooser_dialog(self,widget):
		res = widget.run()
		if res == gtk.RESPONSE_OK:
			self.config_file = widget.get_filename()  #include the pathname
			self.load_conf(self.config_file,conf)
		widget.hide()'''

	def filechooser_clicked(self, widget):
		file_name = widget.get_filename()
		if self.load_conf(file_name,conf) != 0:
			return -1
		self.set_configure(conf)
		print str(file_name)
		
	
	def on_change(self,widget):
		self.label.set_label(widget.get_active_text())

	def connect_clicked(self,widget):
		if self.combobox_com.get_active_text() == None:
			self.display_error(self.error_dialog,"No serial has been choosed !")
		else:
			try:
				self.COM.set_option(self.combobox_com.get_active_text(),self.combobox_baud.get_active_text())
				if self.COM.start():
					widget.set_sensitive(False)
					self.combobox_com.set_sensitive(False)   
					self.disconnect_but.set_sensitive(True)
					self.set_widget(True)
					self.image_status.set_from_pixbuf(self.connect_on)
					self.label.set_label("COM is "+ self.combobox_com.get_active_text()+'''\nBaud is '''+self.combobox_baud.get_active_text())
					self.statusbar.push(0,"")
					self.send([0xff,0xef,0xff,0xff,0xff,0xff,0x75,0xff],"set")
				else:
					return
			except Exception,se:
				self.COM.stop()
				self.display_error(self.error_dialog,"Can not open the "+ self.combobox_com.get_active_text()+" :Access denied!")

	def disconnect_clicked(self,widget):
		try:
			self.COM.stop()
		except Exception,s:
			self.display_error(self.error_dialog,str(s))  
		widget.set_sensitive(False)
		self.set_widget(False)
		self.combobox_com.set_sensitive(True)
		self.connect_but.set_sensitive(True)
		self.image_status.set_from_pixbuf(self.connect_off)
		self.label.set_label("COM is "+'''\n Baud is''')

	def buttonsend_clicked(self, widget):
		response = self.send_dialog.run()
		self.send_dialog.hide()
		if response == gtk.RESPONSE_CANCEL:#cancel
			#print "cancel the send "
			return
		elif response == -12:#load file
			#self.display_filechooser_dialog(self.filechooser_dialog)
			res = self.filechooser_dialog.run()
			self.filechooser_dialog.hide()
			if res == gtk.RESPONSE_OK:
				self.config_file = self.filechooser_dialog.get_filename()  #include the pathname
				if self.load_conf(self.config_file,conf) != 0:
					return -1
			else:
				return
		else :
			return
		self.messagedialog_input.set_title("Input the cofigue name...")
		self.messagedialog_input.set_markup("\n\nInput the cofigue name (default C131)")
		response = self.messagedialog_input.run()
		self.messagedialog_input.hide()
		if response == gtk.RESPONSE_CANCEL:#cancel
			print "cancel the send "
			return
		elif response == gtk.RESPONSE_YES:
			send_stream[2] = self.entry_messagedialog.get_text()
			if send_stream[2]== "":
				send_stream[2] = "C131"
		else:
			return
		self.send(tmp_data,"add")
		self.messagedialog.set_title("Update relay status")
		self.messagedialog.set_markup("\n\nUpdate current relay status?)")
		response = self.messagedialog.run()
		self.messagedialog.hide()
		if response == gtk.RESPONSE_CANCEL:#cancel
			print "cancel the Update "
			return
		elif response == gtk.RESPONSE_YES:
			self.send(tmp_data,"set")
			return

	def buttonsave_clicked(self, widget, sort):
		response = self.dialog_save.run()
		self.dialog_save.hide()
		if response == gtk.RESPONSE_ACCEPT:
			config_file = self.widget_save.get_filename()
			if sort == "configure":
				self.save_configure(conf,config_file)
			elif sort == "diag":
				self.save_sdm_diag(config_file)
				pass
		print "in save button  clicked "

	def buttongetcfg_clicked(self,widget):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please Open Serial Port!")
			return
		self.COM.send("apt get cfg")
		self.COM.read()
		self.COM.send("\r")
		string = self.COM.read()
		spit_list = string.splitlines()
		print spit_list[2]
		if spit_list[2] == "No cfg":
			self.display_error(self.error_dialog,"No cfg be choosed!","Get cfg...")
		else :
			#print tmp_data
			spit__list = spit_list[2].split()
			if len(spit__list) != 8:
				self.display_error(self.error_dialog,"Load config fail!\n Please check the connect!","Load Failed")
				return -1
			tmp = int(spit__list[0],16)
			for n in range(8):
				if tmp & (0x01 << n):
					conf['loop'+str(n+1)] = "on"
				else:
					conf['loop'+str(n+1)] = "off"

			tmp = int(spit__list[1],16)
			for n in range(4):
				if  tmp & (0x01 << n):
					conf['loop'+str(n+9)] = "on"
				else:
					conf['loop'+str(n+9)] = "off"
			i = 11
			for n in (5,6):
				if tmp & (0x01 << n) :
					conf['loop'+str(i)+'b'] = "on"
				else:
					conf['loop'+str(i)+'b'] = "off"
				i = i+1

			tmp = int(spit__list[2],16)
			for n in range(6):
				if tmp & (0x01 << n):
					conf['ess'+str(n+1)] = "on"
				else :
					conf['ess'+str(n+1)] = "off"

			tmp = int(spit__list[4],16)
			i=0
			for n in (1,2,3,5,6):
				i = i+1
				if tmp & (0x01 << n):
					conf['led'+str(i)] = "on"
				else :
					conf['led'+str(i)] = "off"
			i = 1
			for n in (0,4):
				if tmp & (0x01 << n):
					conf['led'+str(i)+'_a'] = "active high"
				else :
					conf['led'+str(i)+'_a'] = "active low"
				i = i+3

			tmp = int(spit__list[6],16)
			i=0
			for n in (0,2,4,5,6):
				i = i+1
				if tmp & (0x01 << n):
					conf['sw'+str(i)] = "on"
				else :
					conf['sw'+str(i)] = "off"
			i = (1,2,5)
                        n = (1,3,7)
                        for rag in range(3):                                    #attention
                                if tmp &(0x01 << n[rag]):
                                        conf['sw'+str(i[rag])+'_s'] = "state1"
                                else:
                                        conf['sw'+str(i[rag])+'_s'] = "state2"    

			self.display_ok(self.ok_dialog,"Get cfg success!")
			self.set_configure(conf)

	def buttonsdmpwr_clicked(self,widget):
		value = ""
		if widget.get_label() == "ON":
			self.image_sdm.set_from_pixbuf(self.sdm_on)
			widget.set_label("OFF")
			value = "on"
		else:
			self.image_sdm.set_from_pixbuf(self.sdm_off)
			widget.set_label("ON")
			value = "off"
		self.send_pwr("sdm",value)

	def buttonsq1_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq1.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop1'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 0)
			else:
				self.image_sq1.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop1'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 0))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq1.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq1.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq2_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq2.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop2'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 1)
			else:
				self.image_sq2.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop2'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 1))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq2.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq2.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq3_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq3.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop3'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 2)
			else:
				self.image_sq3.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop3'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 2))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq3.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq3.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq4_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq4.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop4'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 3)
			else:
				self.image_sq4.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop4'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 3))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq4.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq4.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq5_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq5.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop5'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 4)
			else:
				self.image_sq5.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop5'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 4))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq5.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq5.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq6_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq6.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop6'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 5)
			else:
				self.image_sq6.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop6'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 5))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq6.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq6.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq7_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq7.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop7'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 6)
			else:
				self.image_sq7.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop7'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 6))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq7.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq7.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq8_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq8.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop8'] = 'on'
				tmp_data[0] = tmp_data[0] | (0x01 << 7)
			else:
				self.image_sq8.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop8'] = 'off'
				tmp_data[0] = tmp_data[0] & (~(0x01 << 7))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq8.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq8.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq9_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq9.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop9'] = 'on'
				tmp_data[1] = tmp_data[1] | (0x01 << 0)
			else:
				self.image_sq9.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop9'] = 'off'
				tmp_data[1] = tmp_data[1] & (~(0x01 << 0))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq9.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq9.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq10_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq10.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop10'] = 'on'
				tmp_data[1] = tmp_data[1] | (0x01 << 1)
			else:
				self.image_sq10.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop10'] = 'off'
				tmp_data[1] = tmp_data[1] & (~(0x01 << 1))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq10.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq10.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass
				
	def buttonsq11_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq11.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop11'] = 'on'
				tmp_data[1] = tmp_data[1] | (0x01 << 2)
			else:
				self.image_sq11.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop11'] = 'off'
				tmp_data[1] = tmp_data[1] & (~(0x01 << 2))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq11.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq11.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq12_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq12.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop12'] = 'on'
				tmp_data[1] = tmp_data[1] | (0x01 << 3)
			else:
				self.image_sq12.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop12'] = 'off'
				tmp_data[1] = tmp_data[1] & (~(0x01 << 3))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq12.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq12.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq11b_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq11b.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop11b'] = 'on'
				tmp_data[1] = tmp_data[1] | (0x01 << 5)
			else:
				self.image_sq11b.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop11b'] = 'off'
				tmp_data[1] = tmp_data[1] & (~(0x01 << 5))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq11b.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq11b.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsq12b_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sq12b.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['loop12b'] = 'on'
				tmp_data[1] = tmp_data[1] | (0x01 << 6)
			else:
				self.image_sq12b.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['loop12b'] = 'off'
				tmp_data[1] = tmp_data[1] & (~(0x01 << 6))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq12b.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq12b.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass
	
	def buttoness1_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_ess1.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['ess1'] = 'on'
				tmp_data[2] = tmp_data[2] | (0x01 << 0)
			else:
				self.image_ess1.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['ess1'] = 'off'
				tmp_data[2] = tmp_data[2] & (~(0x01 << 0))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_ess1.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_ess1.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttoness2_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_ess2.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['ess2'] = 'on'
				tmp_data[2] = tmp_data[2] | (0x01 << 1)
			else:
				self.image_ess2.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['ess2'] = 'off'
				tmp_data[2] = tmp_data[2] & (~(0x01 << 1))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_ess2.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_ess2.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttoness3_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_ess3.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['ess3'] = 'on'
				tmp_data[2] = tmp_data[2] | (0x01 << 2)
			else:
				self.image_ess3.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['ess3'] = 'off'
				tmp_data[2] = tmp_data[2] & (~(0x01 << 2))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_ess3.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_ess3.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttoness4_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_ess4.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['ess4'] = 'on'
				tmp_data[2] = tmp_data[2] | (0x01 << 3)
			else:
				self.image_ess4.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['ess4'] = 'off'
				tmp_data[2] = tmp_data[2] & (~(0x01 << 3))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_ess4.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_ess4.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttoness5_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_ess5.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['ess5'] = 'on'
				tmp_data[2] = tmp_data[2] | (0x01 << 4)
			else:
				self.image_ess5.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['ess5'] = 'off'
				tmp_data[2] = tmp_data[2] & (~(0x01 << 4))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_ess5.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_ess5.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass
		
	def buttoness6_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_ess6.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['ess6'] = 'on'
				tmp_data[2] = tmp_data[2] | (0x01 << 5)
			else:
				self.image_ess6.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['ess6'] = 'off'
				tmp_data[2] = tmp_data[2] & (~(0x01 << 5))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_ess6.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_ess6.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonlamp1_clicked(self,widget,value):
		if(value == None):
                        #self.waiting.show()
			if widget.get_label() == "    ON    ":
				self.image_lamp1.set_from_pixbuf(self.led_on)
				widget.set_label("    OFF   ")
				conf['led1'] = 'on'
				tmp_data[4] = tmp_data[4] | (0x01 << 1)
			else:
				self.image_lamp1.set_from_pixbuf(self.led_off)
				widget.set_label("    ON    ")
				conf['led1'] = 'off'
				tmp_data[4] = tmp_data[4] & (~(0x01 << 1))
				#self.waiting.close()
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_lamp1.set_from_pixbuf(self.led_on)
			widget.set_label("    OFF   ")
		elif (value == "off"):
			self.image_lamp1.set_from_pixbuf(self.led_off)
			widget.set_label("    ON    ")
		else:
			pass                

	def buttonlamp2_clicked(self,widget,value):
		if(value == None):
			if widget.get_label() == "    ON    ":
				self.image_lamp2.set_from_pixbuf(self.led_on)
				widget.set_label("    OFF   ")
				conf['led2'] = 'on'
				tmp_data[4] = tmp_data[4] | (0x01 << 2)
			else:
				self.image_lamp2.set_from_pixbuf(self.led_off)
				widget.set_label("    ON    ")
				conf['led2'] = 'off'
				tmp_data[4] = tmp_data[4] & (~(0x01 << 2))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_lamp2.set_from_pixbuf(self.led_on)
			widget.set_label("    OFF   ")
		elif (value == "off"):
			self.image_lamp2.set_from_pixbuf(self.led_off)
			widget.set_label("    ON    ")
		else:
			pass

	def buttonlamp3_clicked(self,widget,value):
		if(value == None):
			if widget.get_label() == "    ON    ":
				self.image_lamp3.set_from_pixbuf(self.led_on)
				widget.set_label("    OFF   ")
				conf['led3'] = 'on'
				tmp_data[4] = tmp_data[4] | (0x01 << 3)
			else:
				self.image_lamp3.set_from_pixbuf(self.led_off)
				widget.set_label("    ON    ")
				conf['led3'] = 'off'
				tmp_data[4] = tmp_data[4] & (~(0x01 << 3))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_lamp3.set_from_pixbuf(self.led_on)
			widget.set_label("    OFF   ")
		elif (value == "off"):
			self.image_lamp3.set_from_pixbuf(self.led_off)
			widget.set_label("    ON    ")
		else:
			pass

	def buttonlamp4_clicked(self,widget,value):
		if(value == None):
			if widget.get_label() == "    ON    ":
				self.image_lamp4.set_from_pixbuf(self.led_on)
				widget.set_label("    OFF   ")
				conf['led4'] = 'on'
				tmp_data[4] = tmp_data[4] | (0x01 << 5)
			else:
				self.image_lamp4.set_from_pixbuf(self.led_off)
				widget.set_label("    ON    ")
				conf['led4'] = 'off'
				tmp_data[4] = tmp_data[4] & (~(0x01 << 5))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_lamp4.set_from_pixbuf(self.led_on)
			widget.set_label("    OFF   ")
		elif (value == "off"):
			self.image_lamp4.set_from_pixbuf(self.led_off)
			widget.set_label("    ON    ")
		else:
			pass

	def buttonlamp5_clicked(self,widget,value):
		if(value == None):
			if widget.get_label() == "    ON    ":
				self.image_lamp5.set_from_pixbuf(self.led_on)
				widget.set_label("    OFF   ")
				conf['led5'] = 'on'
				tmp_data[4] = tmp_data[4] | (0x01 << 6)
			else:
				self.image_lamp5.set_from_pixbuf(self.led_off)
				widget.set_label("    ON    ")
				conf['led5'] = 'off'
				tmp_data[4] = tmp_data[4] & (~(0x01 << 6))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_lamp5.set_from_pixbuf(self.led_on)
			widget.set_label("    OFF   ")
		elif (value == "off"):
			self.image_lamp5.set_from_pixbuf(self.led_off)
			widget.set_label("    ON    ")
		else:
			pass

	def buttonsw1_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sw1.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['sw1'] = 'on'
				tmp_data[6] = tmp_data[6] | (0x01 << 0)
			else:
				self.image_sw1.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['sw1'] = 'off'
				tmp_data[6] = tmp_data[6] & (~(0x01 << 0))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sw1.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sw1.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsw2_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sw2.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['sw2'] = 'on'
				tmp_data[6] = tmp_data[6] | (0x01 << 2)
			else:
				self.image_sw2.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['sw2'] = 'off'
				tmp_data[6] = tmp_data[6] & (~(0x01 << 2))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sw2.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sw2.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsw3_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sw3.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['sw3'] = 'on'
				tmp_data[6] = tmp_data[6] | (0x01 << 4)
			else:
				self.image_sw3.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['sw3'] = 'off'
				tmp_data[6] = tmp_data[6] & (~(0x01 << 4))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sw3.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sw3.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsw4_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sw4.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['sw4'] = 'on'
				tmp_data[6] = tmp_data[6] | (0x01 << 5)
			else:
				self.image_sw4.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['sw4'] = 'off'
				tmp_data[6] = tmp_data[6] & (~(0x01 << 5))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sw4.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sw4.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def buttonsw5_clicked(self,widget,value):
		if(value == None):         
			if widget.get_label() == "   Open   ":
				self.image_sw5.set_from_pixbuf(self.switch_on)
				widget.set_label("   Close  ")
				conf['sw5'] = 'on'
				tmp_data[6] = tmp_data[6] | (0x01 << 6)
			else:
				self.image_sw5.set_from_pixbuf(self.switch_off)
				widget.set_label("   Open   ")
				conf['sw5'] = 'off'
				tmp_data[6] = tmp_data[6] & (~(0x01 << 6))
			#self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sw5.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sw5.set_from_pixbuf(self.switch_off)
			widget.set_label("   Open   ")
		else:
			pass

	def set_widget(self,value):
		name = "combobox_sw"
		string = ""
		for n in (1,2,5):
			string = "self."+ name + str(n)+"(" + str(value)+")"
			#print string
			
		self.button_sdmpwr.set_sensitive(value)
		#self.radio_exte.set_sensitive(value)
		#self.radio_inst.set_sensitive(value)
		self.button_send.set_sensitive(value)

	def set_configure(self,conf):
		for n in range(12):
			string = "buttonsq"
			string = "self."+string+str(n+1)+"_clicked("+"self.button_sq"+str(n+1)+","+"""'"""+conf['loop'+str(n+1)]+"""'"""+")"
			eval(string)
		for n in range(2):
			string = "buttonsq"
			string = "self."+string+str(n+11)+"b_clicked("+"self.button_sq"+str(n+11)+"b,"+"""'"""+conf['loop'+str(n+11)+"b"]+"""'"""+")"
			eval(string)
		for n in range(5):
			string = "buttonlamp"
			string = "self."+string+str(n+1)+"_clicked("+"self.button_lamp"+str(n+1)+","+"""'"""+conf['led'+str(n+1)]+"""'"""+")"
			eval(string)
		for n in range(6):
			string = "buttoness"
			string = "self."+string+str(n+1)+"_clicked("+"self.button_ess"+str(n+1)+","+"""'"""+conf['ess'+str(n+1)]+"""'"""+")"
			eval(string)
		for n in range(5):
			string = "buttonsw"
			string = "self."+string+str(n+1)+"_clicked("+"self.button_sw"+str(n+1)+","+"""'"""+conf['sw'+str(n+1)]+"""'"""+")"
			eval(string)
		##self.radio_ledpwr(conf['led_pwr'])
		self.led1combobox_change(self.combobox_led1 ,conf['led1_a'])
		self.led4combobox_change(self.combobox_led4 ,conf['led4_a'])
		self.sw1combobox_change(self.combobox_sw1 ,conf['sw1_s'])
		self.sw2combobox_change(self.combobox_sw2 ,conf['sw2_s'])
		self.sw5combobox_change(self.combobox_sw5 ,conf['sw5_s'])
		

		"""
			if conf['loop'+str(n+1)] == "off":
				eval(string(self,widget))
			elif conf['loop'] == "on":
				self.combobox_led1.set_active(1)
		"""

	def save_configure(self,conf,filename):
		#if os.path.exists(filename):                #dudge the file is  or not exists
			#print filename
		#else:
		print filename+"new file \n"
		f=open(filename,"w")
		try:
			f.write("###  CONFIG DATA LOG  ###\n")
			f.write(time.strftime('###  %Y-%m-%d  %H:%M:%S  ###',time.localtime(time.time()))+"\n\n")
			#f.write("[loop]\n")
			#for n in range(12):
			#	f.write("loop"+str(n+1) +" = \n")
			#f.write("loop11b = \n")
			#f.write("loop12b = \n")
			#f.write("\n\n[ess]\n")
			#for n in range(6):
			#	f.write("ess"+str(n+1) +" = \n")
			#f.write("\n\n[led]\n")
			#for n in range(5):
			#	f.write("led"+str(n+1) +" = \n")
			#f.write("led1_a = \n")
			#f.write("led4_a = \n")
			#f.write("\n\n[sw]\n")
			#for n in range(5):
			#	f.write("sw"+str(n+1) +" = \n")
			f.close()
			self.config.read(filename)     #read configure file
			self.remove_all_section()
			#if not(self.config.has_section("loop")):
			self.config.add_section("loop")
			self.config.set("loop","loop1",conf['loop1'])
			self.config.set("loop","loop2",conf['loop2'])
			self.config.set("loop","loop3",conf['loop3'])
			self.config.set("loop","loop4",conf['loop4'])
			self.config.set("loop","loop5",conf['loop5'])
			self.config.set("loop","loop6",conf['loop6'])
			self.config.set("loop","loop7",conf['loop7'])
			self.config.set("loop","loop8",conf['loop8'])
			self.config.set("loop","loop9",conf['loop9'])
			self.config.set("loop","loop10",conf['loop10'])
			self.config.set("loop","loop11",conf['loop11'])
			self.config.set("loop","loop12",conf['loop12'])
			self.config.set("loop","loop11b",conf['loop11b'])
			self.config.set("loop","loop12b",conf['loop12b'])
			#if not(self.config.has_section("ess")):
			self.config.add_section("ess")
			self.config.set("ess","ess1",conf['ess1'])
			self.config.set("ess","ess2",conf['ess2'])
			self.config.set("ess","ess3",conf['ess3'])
			self.config.set("ess","ess4",conf['ess4'])
			self.config.set("ess","ess5",conf['ess5'])
			self.config.set("ess","ess6",conf['ess6'])
			#if not(self.config.has_section("led")):
			self.config.add_section("led")
			self.config.set("led","led1",conf['led1'])
			self.config.set("led","led2",conf['led2'])
			self.config.set("led","led3",conf['led3'])
			self.config.set("led","led4",conf['led4'])
			self.config.set("led","led5",conf['led5'])
			self.config.set("led","led1_a",conf['led1_a'])
			self.config.set("led","led4_a",conf['led4_a'])
			#if not(self.config.has_section("sw")):
			self.config.add_section("sw")
			self.config.set("sw","sw1",conf['sw1'])
			self.config.set("sw","sw2",conf['sw2'])
			self.config.set("sw","sw3",conf['sw3'])
			self.config.set("sw","sw4",conf['sw4'])
			self.config.set("sw","sw5",conf['sw5'])
			self.config.set("sw","sw1_s",conf['sw1_s'])
			self.config.set("sw","sw2_s",conf['sw2_s'])
			self.config.set("sw","sw5_s",conf['sw5_s'])
			f=open(filename,'a')
			self.config.write(f)
			self.display_ok(self.ok_dialog,"Write config file success!")
		except:
			self.display_error(self.error_dialog,"Write config file fail!","Write Failed")

	def save_sdm_diag(self,filename):
		f=open(filename,'w')
		f.write("###  SDM DATA LOG  ###\n")
		f.write(time.strftime('%Y-%m-%d  %H:%M:%S',time.localtime(time.time()))+"\n\n")
		for n in range(4):
			str_temp = "datalog" + str(n)
			for act in eval(str_temp):
				f.write("["+act[0]+"]\n")
				if act[1] == "********":
					f.write("\n")
				else:
					f.write(act[1]+"\n")
		f.write("[DTC]\n"+ dtc_infor[0])
		self.display_ok(self.ok_dialog,"Save SDM Diag Over!")
		f.close()

	def sw1combobox_change(self,widget,value):
		print widget.get_active_text()
		if (value == None):
                        if widget.get_active()==0:
                                conf['sw1_s'] = 'state1'
                                tmp_data[6] = tmp_data[6] & (~(0x01 << 1))
                        elif widget.get_active()==1:
                                conf['sw1_s'] = 'state2'
                                tmp_data[6] = tmp_data[6] | (0x01 << 1)
                        else:
                                pass
		elif (value == "state1"):
			widget.set_active(0)
		elif (value == "state2"):
			widget.set_active(1)
		else:
			pass
		#self.send(tmp_data,"set")

	def sw2combobox_change(self,widget,value):
                print widget.get_active_text()
		if (value == None):
                        if widget.get_active()==0:
                                conf['sw2_s'] = 'state1'
                                tmp_data[6] = tmp_data[6] | (0x01 << 3)#changes state1 and state2
                        elif widget.get_active()==1:
                                conf['sw2_s'] = 'state2'
                                tmp_data[6] = tmp_data[6] & (~(0x01 << 3))
                        else:
                                pass
		elif (value == "state1"):
			widget.set_active(0)
		elif (value == "state2"):
			widget.set_active(1)
		else:
			pass
		#self.send(tmp_data,"set")

	def sw5combobox_change(self,widget,value):
		print widget.get_active_text()
		if (value == None):
                        if widget.get_active()==0:
                                conf['sw5_s'] = 'state1'
                                tmp_data[6] = tmp_data[6] & (~(0x01 << 7))
                        elif widget.get_active()==1:
                                conf['sw5_s'] = 'state2'
                                tmp_data[6] = tmp_data[6] | (0x01 << 7)
                        else:
                                pass
		elif (value == "state1"):
			widget.set_active(0)
		elif (value == "state2"):
			widget.set_active(1)
		else:
			pass
		#self.send(tmp_data,"set")

	def led1combobox_change(self,widget,value):
                print value
		if (value == None):
			if widget.get_active()== 0:
				conf['led1_a'] = 'active low'
				#print conf['led1_a']
				tmp_data[4] = tmp_data[4] & (~(0x01 << 0))
			elif widget.get_active()== 1:
				conf['led1_a'] = 'active high'
				#print conf['led1_a']
				tmp_data[4] = tmp_data[4] | (0x01 << 0)
			#self.send(tmp_data,"set")
		elif (value == "active low"):
			widget.set_active(0)
		elif (value == "active high"):
			widget.set_active(1)
		else:
			pass

	def led4combobox_change(self,widget,value):
                print value
		if (value == None):
			if widget.get_active()== 0:
				conf['led4_a'] = 'active low'
				#print conf['led4_a']
				tmp_data[4] = tmp_data[4] & (~(0x01 << 4))
			elif widget.get_active() == 1:
				conf['led4_a'] = 'active high'
				#print conf['led4_a']
				tmp_data[4] = tmp_data[4] | (0x01 << 4)
			#self.send(tmp_data,"set")
		elif (value == "active low"):
			widget.set_active(0)
		elif (value == "active high"):
			widget.set_active(1)
		else:
			pass
	def send(self,data,style):
		string_temp = ""
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please make sure  the serial is opened!","Error","serial")
		elif self.COM.get_option()==True:
				for n in range(8):
					temp = data[n] & 0x0f
					if(temp > 9 ):
						send_stream[3+n*2] = temp + 87  ##0x44--> '4' '4'   0x61(a)
					else:
						send_stream[3+n*2] = temp + 48  
					temp = (data[n] >> 4) & 0x0f
					if(temp > 9 ):
						send_stream[4+n*2] = temp +  87 
					else:
						send_stream[4+n*2] = temp + 48                
				for n in range(3):
					if (style == "add" and n == 1):
						self.COM.send(config_stream[1])
						string_temp = string_temp + config_stream[1] + " "
					else:
						self.COM.send(send_stream[n])
						string_temp = string_temp + send_stream[n] + " "
					self.COM.send(" ")
				for n in range(8):
					self.COM.send(chr(send_stream[2*n+4]))
					self.COM.send(chr(send_stream[2*n+3]))
					string_temp = string_temp + chr(send_stream[2*n+4]) + chr(send_stream[2*n+3]) + " "
					self.COM.send(" ")
				self.COM.send("\r")
				print string_temp
	
	def send_pwr(self,style,value):
			send_pwr_stream[2] = value
			send_pwr_stream[3] = style
			for n in range(4):
				self.COM.send(send_pwr_stream[n])
				self.COM.send(" ")
			self.COM.send("\r")

if __name__== "__main__":
	Glade_main()
	gtk.main()
