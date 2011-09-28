#!/user/bin/env python

conf = {"loop1":"on","loop2":"on","loop3":"on","loop4":"on","loop5":"on","loop6":"on",\
		"loop7":"on","loop8":"on","loop9":"on","loop10":"on","loop11":"on","loop12":"on",\
		"ess1":"on","ess2":"on","ess3":"on","ess4":"on","ess5":"on","ess6":"on",\
		"led1":"on","led2":"on","led3":"on","led4":"on","led5":"on",\
		"led1_a":"active low","led4_a":"active low",\
		"sw1":"on","sw2":"on","sw3":"on","sw4":"on","sw5":"on"}

config_stream = "apt","add"
set_stream = "apt","set"
tmp_data = [0xff,0xef,0xff,0xff,0xff,0xff,0x75,0xff]
send_stream = [set_stream[0],set_stream[1],"apt","f","f","e","f",  "f","f","f","f",   "f","f","f","f",   "7","5","f","f"]  #apt + add + name + byte(0-7)
pwr_stream = "apt","pwr"
send_pwr_stream = [pwr_stream[0],pwr_stream[1],"off","led"]

#import cairo
import ConfigParser
import sys
import os
import apt_serial
import gobject
import scan

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

#def dec_com:

class Glade_main(gtk.Window):
	def __init__(self):
		global conf   
		main_dir = os.path.dirname(__file__)
		glade_file = os.path.join(main_dir, "apt_main.glade")
		self.config_file = os.path.join(main_dir, "apt_config.ini")
		self.config = ConfigParser.ConfigParser()
		self.builder = gtk.Builder()
		self.builder.add_from_file(glade_file)
		self.windows = self.builder.get_object("MainWindow")        
		self.about_dialog = self.builder.get_object("about_dialog")
		self.help_about = self.builder.get_object("help_about")
		self.error_dialog = self.builder.get_object("error_dialog")
		self.send_dialog = self.builder.get_object("dialog_send")
		self.filechooser_dialog = self.builder.get_object("filechooser_dialog")
		self.dialog_save = self.builder.get_object("dialog_save")
		self.widget_save = self.builder.get_object("widget_save")
		self.menu_quit = self.builder.get_object("menuitem_quit")
		
		self.scan=scan.scan()
		self.combobox_com=self.builder.get_object("combo_com")
		liststore_com = gtk.ListStore(str)
		cell_com = gtk.CellRendererText()
		self.combobox_com.pack_start(cell_com)
		self.combobox_com.add_attribute(cell_com, 'text', 0)
		for n,s in self.scan:
			print "(%d) %s" % (n,s)
			liststore_com.append([s])
		#liststore_com.append(["COM2"])
		self.combobox_com.set_model(liststore_com)
		self.combobox_com.set_active(0)


		self.combobox_baud=self.builder.get_object("combo_baud")
		liststore_baud = gtk.ListStore(str)
		cell_baud = gtk.CellRendererText()
		self.combobox_baud.pack_start(cell_baud)
		self.combobox_baud.add_attribute(cell_baud, 'text', 0)
		#self.combobox_baud.set_wrap_width(-1)
		#for n in range(50):
		 #   liststore.append(['Ite %d'%n])
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
		self.combobox_sw1.connect("changed",self.sw1combobox_change)

		self.combobox_sw2 = self.builder.get_object("combo_sw2")
		self.combobox_sw2.pack_start(cell_sw1)
		self.combobox_sw2.add_attribute(cell_sw1, 'text', 0)
		self.combobox_sw2.set_model(liststore_sw1)
		self.combobox_sw2.set_active(0)
		self.combobox_sw2.connect("changed",self.sw2combobox_change)

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
		self.combobox_sw5.connect("changed",self.sw5combobox_change)

		self.label=self.builder.get_object("label")
		self.label.set_label("COM is "+'''\n Baud is''')
		self.connect_but = self.builder.get_object("connect_but")
		self.disconnect_but = self.builder.get_object("disconnect_but")
		self.button_quit = self.builder.get_object("button_quit")
		self.button_sdmpwr = self.builder.get_object("button_sdmpwr")
		self.button_save = self.builder.get_object("button_save")
		self.button_filechooser = self.builder.get_object("button_filechooser")
		filter = gtk.FileFilter()
		filter.set_name("Configure file(*.ini)")
		filter.add_pattern("*.ini")
		self.button_filechooser.add_filter(filter)
		self.filechooser_dialog.add_filter(filter)
		self.widget_save.add_filter(filter)

		self.button_filechooser.connect("file-set",self.filechooser_clicked)
		
		self.radio_exte = self.builder.get_object("radio_exte")
		self.radio_inst = self.builder.get_object("radio_inst")
		

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
		
		
		self.led_off = gtk.gdk.pixbuf_new_from_file("led_off.png")
		self.led_on = gtk.gdk.pixbuf_new_from_file("led_on.png")
		self.connect_on = gtk.gdk.pixbuf_new_from_file("connect_on.png")
		self.connect_off = gtk.gdk.pixbuf_new_from_file("connect_off.png")   
		self.sdm_on = gtk.gdk.pixbuf_new_from_file("sdm_on.png")
		self.sdm_off = gtk.gdk.pixbuf_new_from_file("sdm_off.png")
		self.switch_on = gtk.gdk.pixbuf_new_from_file("switch_on.png")
		self.switch_off = gtk.gdk.pixbuf_new_from_file("switch_off.png")
		#self.image_lamp.set_from_pixbuf(self.led_off)
		  
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
		self.button_save.connect("clicked",self.buttonsave_clicked)
		
		self.radio_exte.connect("pressed", self.radio_ledpwr_exte)
		self.radio_inst.connect("pressed", self.radio_ledpwr_inst)
		self.set_widget(False)
		self.set_configure(conf)
		self.windows.show()
   
	def on_about_dialog (self, widget):
		self.about_dialog.run()
		self.about_dialog.hide()

	def display_error(self, widget, text):
		error_text = text
		self.error_dialog.set_markup(error_text)
		self.error_dialog.run()
		self.error_dialog.hide()
	def load_conf(self,file_name,conf):
		self.config.read(file_name)     #read configure file
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
		for n in range(8):
			if conf['loop'+str(n+1)] == "on":
				tmp_data[0] = tmp_data[0] | (0x01 << n)
			elif conf['loop'+str(n+1)] == "off":
				tmp_data[0] = tmp_data[0] & (~(0x01 << n))
		print "%x" %(tmp_data[0])
		for n in range(4):
			if conf['loop'+str(n+9)] == "on":
				tmp_data[1] = tmp_data[1] | (0x01 << n)
			elif conf['loop'+str(n+9)] == "off":
				tmp_data[1] = tmp_data[1] & (~(0x01 << n))
		print "%x" %(tmp_data[1])

		conf['ess1'] = self.config.get("ess", "ess1")
		conf['ess2'] = self.config.get("ess", "ess2")
		conf['ess3'] = self.config.get("ess", "ess3")
		conf['ess4'] = self.config.get("ess", "ess4")
		conf['ess5'] = self.config.get("ess", "ess5")
		conf['ess6'] = self.config.get("ess", "ess6")
		for n in range(6):
			if conf['ess'+str(n+1)] == "on":
				tmp_data[2] = tmp_data[2] | (0x01 << n)
			elif conf['ess'+str(n+1)] == "off":
				tmp_data[2] = tmp_data[2] & (~(0x01 << n))
		print "%x" %(tmp_data[2])            

		conf['led1'] = self.config.get("led", "led1")
		conf['led2'] = self.config.get("led", "led1")
		conf['led3'] = self.config.get("led", "led1")
		conf['led4'] = self.config.get("led", "led1")
		conf['led5'] = self.config.get("led", "led1")
		conf['led1_a'] = self.config.get("led", "led1_a")
		conf['led4_a'] = self.config.get("led", "led4_a")
		i=0
		for n in (1,2,3,5,6):
			i = i+1
			if conf['led'+str(i)] == "on":
				tmp_data[4] = tmp_data[4] | (0x01 << n)
			elif conf['led'+str(i)] == "off":
				tmp_data[4] = tmp_data[4] & (~(0x01 << n))
		print "%x" %(tmp_data[4]) 
		i=1
		for n in (0,4):
			if conf['led'+str(i)+'_a'] == "active low":
				tmp_data[4] = tmp_data[4] & (~(0x01 << n))
			elif conf['led'+str(i)+'_a'] == "active high":
				tmp_data[4] = tmp_data[4] | (0x01 << n)
			i=i+3
		print "%x" %(tmp_data[4]) 

		

		#conf['led_pwr'] = self.config.get("pwr", "led_pwr")  #it is not belong the config 
		conf['sw1'] = self.config.get("sw", "sw1")
		conf['sw2'] = self.config.get("sw", "sw2")
		conf['sw3'] = self.config.get("sw", "sw3")
		conf['sw4'] = self.config.get("sw", "sw4")
		conf['sw5'] = self.config.get("sw", "sw5")
		i=0
		for n in (0,2,4,5,6):
			i = i+1
			if conf['sw'+str(i)] == "on":
				tmp_data[6] = tmp_data[6] | (0x01 << n)
			elif conf['sw'+str(i)] == "off":
				tmp_data[6] = tmp_data[6] & (~(0x01 << n))
		print "%x" %(tmp_data[6]) 

		print conf
		

	def display_filechooser_dialog(self,widget):
		res = widget.run()
		if res == gtk.RESPONSE_OK:
			self.config_file = widget.get_filename()  #include the pathname
			self.load_conf(self.config_file,conf)
		widget.hide()
		
	'''
	def combox_filetypeclicked(self, widget):
		#string = widget.get_active()
		if widget.get_active()==0:
			print "0"
		elif widget.get_active()==1:
			print "1"         
		#print "success:",string
	'''

	def filechooser_clicked(self, widget):
		file_name = widget.get_filename()
		self.load_conf(file_name,conf)
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
					#self.combobox_baud.set_sensitive(False) #because we do not hope client can choose the baud rate
					self.disconnect_but.set_sensitive(True)
					self.set_widget(True)
					self.image_status.set_from_pixbuf(self.connect_on)
					self.label.set_label("COM is "+ self.combobox_com.get_active_text()+'''\nBaud is '''+self.combobox_baud.get_active_text())
				else:
					pass
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
		#self.combobox_baud.set_sensitive(True) #because we do not hope client can choose the baud rate
		self.connect_but.set_sensitive(True)
		self.image_status.set_from_pixbuf(self.connect_off)
		self.label.set_label("COM is "+'''\n Baud is''')


	def radio_ledpwr_inst(self,widget):
		print "inst was toggled %s" % ( ("ON", "OFF")[widget.get_active()])
		self.send_pwr("led","off")


	def radio_ledpwr_exte(self,widget):
		print "exte was toggled %s" % ( ("ON", "OFF")[widget.get_active()])
		self.send_pwr("led","on")

	def radio_ledpwr(self,value):
		if (value == "inst"):
			self.radio_inst.set_active(True)
		elif (value == "exte"):
			self.radio_exte.set_active(True)
		else:
			pass
		
	def buttonsend_clicked(self, widget):
		response = self.send_dialog.run()
		if response == -13:#current config
			self.send(tmp_data,"add")
			print "XXXXXXXXXXXXXXXX"
		elif response == -12:#load file
			self.send_dialog.hide()
			print "YYYYYYYYYYYYYYYYY"
			self.display_filechooser_dialog(self.filechooser_dialog)
			#return
		else:
			pass
		#self.send_dialog.destroy()
		self.send_dialog.hide()

	def buttonsave_clicked(self, widget):
		response = self.dialog_save.run()
		if response == gtk.RESPONSE_ACCEPT:
			config_file = self.widget_save.get_filename()
			#print config_file
			self.save_configure(conf,config_file)
		self.dialog_save.hide()
		print "in save button  clicked "


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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
		elif (value == "on"):
			self.image_sq12.set_from_pixbuf(self.switch_on)
			widget.set_label("   Close  ")
		elif (value == "off"):
			self.image_sq12.set_from_pixbuf(self.switch_off)
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			self.send(tmp_data,"set")
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
			string = "self."+ name + str(n)+".set_sensitive(" + str(value)+")"
			eval(string)
		self.button_sdmpwr.set_sensitive(value)
		self.radio_exte.set_sensitive(value)
		self.radio_inst.set_sensitive(value)
		'''
		for n in range(5):
			string = "self."+ name + str(n+1)+".set_sensitive(" + str(value)+")"
			#print string
			eval(string)
		name = "button_sq"
		for n in range(12):
			string = "self."+ name + str(n+1)+".set_sensitive(" + str(value)+")"
			#print string
			eval(string)
		name = "button_ess"
		for n in range(6):
			string = "self."+ name + str(n+1)+".set_sensitive(" + str(value)+")"
			eval(string)
		name = "button_sw"
		for n in range(5):
			string = "self."+ name + str(n+1)+".set_sensitive(" + str(value)+")"
			eval(string)
		string = "self.button_send.set_sensitive("+str(value)+")"
		eval(string)
		string = "self.button_sdmpwr.set_sensitive("+str(value)+")"
		eval(string)
		'''
		
	def set_configure(self,conf):
		for n in range(12):
			string = "buttonsq"
			string = "self."+string+str(n+1)+"_clicked("+"self.button_sq"+str(n+1)+","+"""'"""+conf['loop'+str(n+1)]+"""'"""+")"
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
			

	
		"""
			if conf['loop'+str(n+1)] == "off":
				eval(string(self,widget))
			elif conf['loop'] == "on":
				self.combobox_led1.set_active(1)
		"""
		'''
		if conf['led_pwr'] == "Inst":
			self.radio_inst.set_active(True)
		elif conf['led_pwr'] == "Extern":
			self.radio_exte.set_active(True)
		'''

		
	def save_configure(self,conf,filename):
		self.config.read(filename)     #read configure file
		print filename
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

		self.config.set("ess","ess1",conf['ess1'])
		self.config.set("ess","ess2",conf['ess2'])
		self.config.set("ess","ess3",conf['ess3'])
		self.config.set("ess","ess4",conf['ess4'])
		self.config.set("ess","ess5",conf['ess5'])
		self.config.set("ess","ess6",conf['ess6'])
				
		self.config.set("led","led1",conf['led1'])
		self.config.set("led","led2",conf['led2'])
		self.config.set("led","led3",conf['led3'])
		self.config.set("led","led4",conf['led4'])
		self.config.set("led","led5",conf['led5'])
		self.config.set("led","led1_a",conf['led1_a'])
		self.config.set("led","led4_a",conf['led4_a'])
		
		#self.config.set("pwr","led_pwr",conf['led_pwr'])
		
		self.config.set("sw","sw1",conf['sw1'])
		self.config.set("sw","sw2",conf['sw2'])
		self.config.set("sw","sw3",conf['sw3'])
		self.config.set("sw","sw4",conf['sw4'])
		self.config.set("sw","sw5",conf['sw5'])
		
		self.config.write(open(filename,"r+"))


	def sw1combobox_change(self,widget):
		print widget.get_active_text()
		if widget.get_active()==0:
			tmp_data[6] = tmp_data[6] & (~(0x01 << 1))
		elif widget.get_active()==1:
			tmp_data[6] = tmp_data[6] | (0x01 << 1)
		else:
			pass
		self.send(tmp_data,"set")
		
	def sw2combobox_change(self,widget):
		print widget.get_active_text()
		if widget.get_active()==0:
			tmp_data[6] = tmp_data[6] & (~(0x01 << 3))
		elif widget.get_active()==1:
			tmp_data[6] = tmp_data[6] | (0x01 << 3)
		else:
			pass
		self.send(tmp_data,"set")

	def sw5combobox_change(self,widget):
		print widget.get_active_text()
		if widget.get_active()==0:
			tmp_data[6] = tmp_data[6] & (~(0x01 << 7))
		elif widget.get_active()==1:
			tmp_data[6] = tmp_data[6] | (0x01 << 7)
		else:
			pass
		self.send(tmp_data,"set")

	def led1combobox_change(self,widget,value):
		if (value == None):
			if widget.get_active()==0:
				conf['led1_a'] = 'active low'
				print conf['led1_a']
				tmp_data[4] = tmp_data[4] & (~(0x01 << 0))
			elif widget.get_active()==1:
				conf['led1_a'] = 'active high'
				print conf['led1_a']
				tmp_data[4] = tmp_data[4] | (0x01 << 0)
			self.send(tmp_data,"set")
		elif (value == "active low"):
			widget.set_active(0)
		elif (value == "active high"):
			widget.set_active(1)
		else:
			pass


	def led4combobox_change(self,widget,value):
		if (value == None):
			if widget.get_active()==0:
				conf['led4_a'] = 'active low'
				print conf['led4_a']
				tmp_data[4] = tmp_data[4] & (~(0x01 << 4))
			elif widget.get_active()==1:
				conf['led4_a'] = 'active high'
				print conf['led4_a']
				tmp_data[4] = tmp_data[4] | (0x01 << 4)
			self.send(tmp_data,"set")
		elif (value == "active low"):
			widget.set_active(0)
		elif (value == "active high"):
			widget.set_active(1)
		else:
			pass
	def send(self,data,style):
		if self.COM.get_option()==False:
			self.display_error(self.error_dialog,"Please make sure  the serial is opened!")
		elif self.COM.get_option()==True:
			#if(style == "set"):
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
					else:
						self.COM.send(send_stream[n])
					print "%s" %(send_stream[n])
					self.COM.send(" ")
					
				for n in range(8):
					self.COM.send(chr(send_stream[2*n+4]))
					print "%c" %(chr(send_stream[2*n+4]))
					self.COM.send(chr(send_stream[2*n+3]))
					print "%c" %(chr(send_stream[2*n+3]))
					self.COM.send(" ")
				self.COM.send("\r\n")
				print "send succeed!!!!!!!!!!!!!!!"
	
	def send_pwr(self,style,value):
			send_pwr_stream[2] = value
			send_pwr_stream[3] = style
			for n in range(4):
				self.COM.send(send_pwr_stream[n])
				self.COM.send(" ")
			self.COM.send("\r\n")
			print "send succeed!!!!!!!!!!!!!!!"

if __name__== "__main__":
	frm=Glade_main()
	gtk.main()
	
