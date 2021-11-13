import tkinter
from tkinter import ttk
from SerialClass import SerialAchieve   # 导入串口通讯类

class MainSerial:
    def __init__(self):
                 # Define serial variables
        self.port = None
        self.band = None
        self.check = None
        self.data = None
        self.stop = None
        self.myserial = None

                 # Initialization Form
        self.mainwin = tkinter.Tk()
        self.mainwin.title
        self.mainwin.geometry("1000x600")

        # Tags
        self.Label1 = tkinter.Label (self.mainwin, text = "serial number:", font = ("Song body", 15))
        self.Label1.place(x = 5,y = 5)
        self.Label2 = tkinter.Label (self.mainwin, text = "baud rate:", font = ("Song body", 15))
        self.Label2.place(x=5, y=45)
        self.Label3 = tkinter.Label (self.mainwin, text = "check bit:", font = ("Song", 15))
        self.Label3.place(x=5, y=85)
        self.Label4 = tkinter.Label (self.mainwin, text = "data bit:", font = ("Song Body", 15))
        self.Label4.place(x=5, y=125)
        self.Label5 = tkinter.Label(self.mainwin,text = "stop bit:", font = ("Song Body", 15))
        self.Label5.place(x = 5,y = 165)

        # Text display, clear send data
        self.Label6 = tkinter.Label(self.mainwin, text= "Send Data:", font = ("Song", 15))
        self.Label6.place(x=270, y=5)

        self.Label7 = tkinter.Label (self.mainwin, text = "Receive data:", font = ("Song Body", 15))
        self.Label7.place(x=270, y=240)

        self.com1value = tkinter.StringVar() # Brought in the form, create a value
        self.combobox_port = ttk.Combobox(self.mainwin, textvariable=self.com1value,
                                          width = 10,font = ("Song body",13))
        # Enter the selected content
        self.combobox_port["value"] = [""] 
        self.combobox_port.place (x = 135, y = 10) # display

        # Baud rate
        self.bandvalue = tkinter.StringVar() # Subscribed in the form, create a value
        self.combobox_band = ttk.Combobox(self.mainwin, textvariable=self.bandvalue, width=10, font=("Song Body", 13))
        # Enter the selected content
        self.combobox_band["value"] = ["4800","9600","14400","19200","38400","57600","115200"]
        self.combobox_band.current (6) # Default 2nd
        self.combobox_band.place (x = 105, y = 45) #

        # Check Digit 
        self.checkvalue = tkinter.StringVar()
        self.combobox_check = ttk.Combobox(self.mainwin, textvariable=self.checkvalue, width=10, font=("Song Body", 13))
        # Enter the selected content
        self.combobox_check ["value"] = ["no passport bit"] 
        self.combobox_check.current(0) # Default 2nd
        self.combobox_check.place(x = 105, y = 85) # display

        # Data Bit
        self.datavalue = tkinter.StringVar () # Brought in the form, create a value
        self.combobox_data = ttk.Combobox(self.mainwin, textvariable=self.datavalue, width=10, font=("Song Body", 13))
        # Enter the selected content
        self.combobox_data["value"] = ["8", "9", "0"] 
        self.combobox_data.current(0) # Default 2nd
        self.combobox_data.place (x = 105, y = 125) #

        self.stopvalue = tkinter.StringVar () 
        self.combobox_stop = ttk.Combobox(self.mainwin, textvariable=self.stopvalue, width=10, font=("Song Body", 13))
        # Enter the selected content
        self.combobox_stop["value"] = ["1", "0"]
        self.combobox_stop.current (0) # Default 2nd
        self.combobox_stop.place (x = 105, y = 165) # display

        # Button display, open the serial port
        self.button_OK  = tkinter.Button (self.mainwin, text = "Open Serial Port",
                                        command = self.button_OK_click, font = ("Song", 13),
                                        width = 10,height = 1)
        self.button_OK.place (x = 5, y = 210) # Display control
        # Close the serial port
        self.button_Cancel = tkinter.Button (self.mainwin, text = "Close Serial Port", # display text
                                                    command = self.button_Cancel_click, font = ("Song", 13),
                    width=10, height=1)
        self.button_Cancel.place (x = 120, y = 210) # Display control

        #Send Data
        self.button_Cancel = tkinter.Button (self.mainwin, text = "Clear Send Data", # Display Text
                                            command = self.button_clcSend_click, font = ("Song", 13),
                                            width=13, height=1)
        self.button_Cancel.place (x = 400, y = 2) # Display control

        # Clear the received data
        self.button_Cancel = tkinter.Button(self.mainwin, text="Clear Receive Data",
                                            command=self.button_clcRece_click, font=("Song", 13),
                                            width=15, height=1)
        self.button_Cancel.place(x=420, y=235) 

        # Send button
        self.button_send = tkinter.Button (self.mainwin, text = "Send", # display text
                                            command = self.button_Send_click, font = ("Song", 13),
                                            width=6, height=1)
        self.button_send.place (x = 5, y = 255) # Display control

        # Receive button
        self.button_send = tkinter.Button (self.mainwin, text = "Receive", # display text
                                                                        command = self.button_Rece_click, font = ("Song", 13),
                            width=6, height=1)
        self.button_send.place (x = 5, y = 310) # Display control

        # Function components of notepad
        self.SendDataView = tkinter.Text(self.mainwin,width = 70,height = 9,
                                            font = ("Song", 13)) # text is actually a text editor
        self.SendDataView.place (x = 270, y = 35) #

        self.ReceDataView = tkinter.Text(self.mainwin, width=70, height=15,
                                                                    font = ("Song", 13)) # text is actually a text editor
        self.ReceDataView.place (x = 270, y = 270) # Display

        # Content sent
        test_str = tkinter.StringVar(value="Hello")
        self.EnTrysend = tkinter.Entry (self.mainwin, width = 13, textvariable = test_str, font = ("Song body", 15))
        self.EnTrysend.place (x = 80, y = 260) # display

        # Get file path
        test_str = tkinter.StringVar(value="Hello")
        self.EnTrysend = tkinter.Entry (self.mainwin, width = 13, textvariable = test_str, font = ("Song body", 15))
        self.EnTrysend.place (x = 80, y = 260) # display

        # Get the parameters of the interface
        self.band = self.combobox_band.get()
        self.check = self.combobox_check.get()
        self.data = self.combobox_data.get()
        self.stop = self.combobox_stop.get()
        print("baud rate:" + self.band)
        self.myserial = SerialAchieve(int(self.band),self.check,self.data,self.stop)

        # Processing the serial port value
        self.port_list = self.myserial.get_port()
        port_str_list = [] # is used to store cutting serial port numbers
        for i in range(len(self.port_list)):
                         # Cut the serial number
            lines = str(self.port_list[i])
            str_list = lines.split(" ")
            port_str_list.append(str_list[0])
            self.combobox_port["value"] = port_str_list
            self.combobox_port.current (0) # Default 2nd

    def show(self):
        self.mainwin.mainloop()

    def button_OK_click(self):
        '''
                 @ Serial port opening function
        :return: 
        '''
        if self.port == None or self.port.isOpen() == False:
            self.myserial.open_port(self.combobox_port.get())
            print ("Open Serial Port Success")
        else:
            pass

    def button_Cancel_click(self):
        self.myserial.delete_port()
        print ("Close Serial Port Success")

    def button_clcSend_click(self):
        self.SendDataView.delete("1.0","end")

    def button_clcRece_click(self):
        self.ReceDataView.delete("1.0", "end")

    def button_Send_click(self):
        try:
            if self.myserial.port.isOpen() == True:
                print ("start sending data")
                send_str1 = self.entrySend.get()
                self.myserial.Write_data(send_str1)
                self.SendDataView.insert(tkinter.INSERT, send_str1+" ")
                print ("Send Data Success")
            else:
                print ("The serial port is not open")
        except:
                print ("Send Failed")
    def button_Rece_click(self):
        try:
            readstr = self.myserial.Read_data()
            self.ReceDataView.insert(tkinter.INSERT, readstr + " ")
        except:
            print ("Read Fails")


if __name__ == '__main__':
    my_ser1 = MainSerial()
    my_ser1.show()