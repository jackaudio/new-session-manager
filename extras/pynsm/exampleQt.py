#! /usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PyNSMClient -  A New Session Manager Client-Library in one file.

The Non-Session-Manager by Jonathan Moore Liles <male@tuxfamily.org>: http://non.tuxfamily.org/nsm/
New Session Manager by Nils Hilbricht et al  https://new-session-manager.jackaudio.org
With help from code fragments from https://github.com/attwad/python-osc ( DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE v2 )

MIT License

Copyright (c) since 2014: Laborejo Software Suite <info@laborejo.org>, All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

import sys #for qt app args.
from os import getpid #we use this as jack meta data
from PyQt5 import QtWidgets, QtCore

#jack only
import ctypes
from random import uniform #to generate samples between -1.0 and 1.0 for Jack.

#nsm only
from nsmclient import NSMClient


########################################################################
#Prepare the Qt Window
########################################################################
class Main(QtWidgets.QWidget):
    def __init__(self, qtApp):
        super().__init__()

        self.qtApp = qtApp
        self.layout = QtWidgets.QVBoxLayout()
        self.layout.setAlignment(QtCore.Qt.AlignTop | QtCore.Qt.AlignLeft)
        self.setLayout(self.layout)

        niceTitle = "PyNSM v2 Example - JACK Noise"
        self.title = QtWidgets.QLabel("")
        self.saved = QtWidgets.QLabel("")
        self._value = QtWidgets.QSlider(orientation = 1) #horizontal
        self._value.setMinimum(0)
        self._value.setMaximum(100)
        self._value.setValue(50) #only visible for the first start.

        self.valueLabel = QtWidgets.QLabel("Noise Volume: " + str(self._value.value()))
        self._value.valueChanged.connect(lambda new: self.valueLabel.setText("Noise Volume: " + str(new)))

        self.layout.addWidget(self.title)
        self.layout.addWidget(self.saved)
        self.layout.addWidget(self._value)
        self.layout.addWidget(self.valueLabel)

        #Prepare the NSM Client
        #This has to be done as soon as possible because NSM provides paths and names for us.
        #and may quit if NSM environment var was not found.
        self.nsmClient = NSMClient(prettyName = niceTitle, #will raise an error and exit if this example is not run from NSM.
                      supportsSaveStatus = True,
                      saveCallback = self.saveCallback,
                      openOrNewCallback = self.openOrNewCallback,
                      exitProgramCallback = self.exitCallback,
                      loggingLevel = "info", #"info" for development or debugging, "error" for production. default is error.
                      )
        #If NSM did not start up properly the program quits here with an error message from NSM.
        #No JACK client gets created, no Qt window can be seen.


        self.title.setText("<b>" + self.nsmClient.ourClientNameUnderNSM +  "</b>")

        self.eventLoop = QtCore.QTimer()
        self.eventLoop.start(100) #10ms-20ms is smooth for "real time" feeling. 100ms is still ok.
        self.eventLoop.timeout.connect(self.nsmClient.reactToMessage)

        #self.show is called as the new/open callback.

    @property
    def value(self):
        return str(self._value.value())

    @value.setter
    def value(self, new):
        new = int(new)
        self._value.setValue(new)

    def saveCallback(self, ourPath, sessionName, ourClientNameUnderNSM): #parameters are filled in by NSM.
        if self.value:
            with open(ourPath, "w") as f:  #ourpath is taken as a filename here. We have this path name at our disposal and we know we only want one file. So we don't make a directory. This way we don't have to create a dir first.
                f.write(self.value)

    def openOrNewCallback(self, ourPath, sessionName, ourClientNameUnderNSM): #parameters are filled in by NSM.
        try:
            with open(ourPath, "r") as f:
                savedValue = f.read() #a string
                self.saved.setText("{}: {}".format(ourPath, savedValue))
                self.value = savedValue #internal casting to int. Sets the slider.
        except FileNotFoundError:
            self.saved.setText("{}: No save file found. Normal for first start.".format(ourPath))
        finally:
            self.show()

    def exitCallback(self, ourPath, sessionName, ourClientNameUnderNSM):
        """This function is a callback for NSM.
        We have a chance to close our clients and open connections here.
        If not nsmclient will just kill us no matter what
        """
        cjack.jack_remove_properties(ctypesJackClient, ctypesJackUuid) #clean our metadata
        cjack.jack_client_close(ctypesJackClient) #omitting this introduces problems. in Jack1 this would mute all jack clients for several seconds.
        exit() #or get SIGKILLed through NSM

    def closeEvent(self, event):
        """Qt likes to quits on its own. For example when the window manager closes the
        main window. Ignore that request and instead send a roundtrip through NSM"""
        self.nsmClient.serverSendExitToSelf()
        event.ignore()

#Prepare the window instance. Gets executed at the end of this file.
qtApp = QtWidgets.QApplication(sys.argv)
ourClient = Main(qtApp)

########################################################################
#Prepare the JACK Client
#We need the client name from NSM first.
########################################################################
cjack = ctypes.cdll.LoadLibrary("libjack.so.0")
clientName = ourClient.nsmClient.prettyName #the nsm client is in the qt instance here. But in your program it can be anywhere.
options = 0
status = None

class jack_client_t(ctypes.Structure):
    _fields_ = []
cjack.jack_client_open.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]  #the two ints are enum and pointer to enum. #http://jackaudio.org/files/docs/html/group__ClientFunctions.html#gab8b16ee616207532d0585d04a0bd1d60
cjack.jack_client_open.restype = ctypes.POINTER(jack_client_t)
ctypesJackClient = cjack.jack_client_open(clientName.encode("ascii"), options, status)

#Create one output port
class jack_port_t(ctypes.Structure):
    _fields_ = []

JACK_DEFAULT_AUDIO_TYPE = "32 bit float mono audio".encode("ascii") #http://jackaudio.org/files/docs/html/types_8h.html
JACK_PORT_IS_OUTPUT = 0x2 #http://jackaudio.org/files/docs/html/types_8h.html
portname = "output".encode("ascii")

cjack.jack_port_register.argtypes = [ctypes.POINTER(jack_client_t), ctypes.c_char_p, ctypes.c_char_p, ctypes.c_ulong, ctypes.c_ulong] #http://jackaudio.org/files/docs/html/group__PortFunctions.html#ga3e21d145c3c82d273a889272f0e405e7
cjack.jack_port_register.restype = ctypes.POINTER(jack_port_t)
outputPort = cjack.jack_port_register(ctypesJackClient, portname,  JACK_DEFAULT_AUDIO_TYPE, JACK_PORT_IS_OUTPUT, 0)

cjack.jack_client_close.argtypes = [ctypes.POINTER(jack_client_t),]

#Create the callback
#http://jackaudio.org/files/docs/html/group__ClientCallbacks.html#gafb5ec9fb4b736606d676c135fb97888b

jack_nframes_t = ctypes.c_uint32
cjack.jack_port_get_buffer.argtypes = [ctypes.POINTER(jack_port_t), jack_nframes_t]
cjack.jack_port_get_buffer.restype = ctypes.POINTER(ctypes.c_float) #this is only valid for audio, not for midi. C Jack has a pointer to void here.

def pythonJackCallback(nframes, void): #types: jack_nframes_t (ctypes.c_uint32), pointer to void
    """http://jackaudio.org/files/docs/html/simple__client_8c.html#a01271cc6cf692278ae35d0062935d7ae"""
    out = cjack.jack_port_get_buffer(outputPort, nframes) #out should be a pointer to jack_default_audio_sample_t (float, ctypes.POINTER(ctypes.c_float))

    #For each required sample
    for i in range(nframes):
        factor = ourClient._value.value() / 100
        val =  ctypes.c_float(round(uniform(-0.5, 0.5) * factor, 10))
        out[i]= val

    return 0 # 0 on success, otherwise a non-zero error code, causing JACK to remove that client from the process() graph.

JACK_CALLBACK_TYPE = ctypes.CFUNCTYPE(ctypes.c_int, jack_nframes_t, ctypes.c_void_p) #the first parameter is the return type, the following are input parameters
callbackFunction = JACK_CALLBACK_TYPE(pythonJackCallback)

cjack.jack_set_process_callback.argtypes = [ctypes.POINTER(jack_client_t), JACK_CALLBACK_TYPE, ctypes.c_void_p]
cjack.jack_set_process_callback.restype = ctypes.c_uint32 #I think this is redundant since ctypes has int as default result type
cjack.jack_set_process_callback(ctypesJackClient, callbackFunction, 0)

#Ready. Activate the client.
cjack.jack_activate(ctypesJackClient)
#The Jack Processing functions gets called by jack in another thread. We just have to keep this program itself running. Qt does the job.


#Jack Metadata - Inform the jack server about our program. Optional but has benefits when used with other programs that rely on metadata.
#http://jackaudio.org/files/docs/html/group__Metadata.html
jack_uuid_t = ctypes.c_uint64
cjack.jack_set_property.argtypes = [ctypes.POINTER(jack_client_t), jack_uuid_t, ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]  #client(we), subject/uuid,key,value/data,type
cjack.jack_remove_properties.argtypes = [ctypes.POINTER(jack_client_t), jack_uuid_t] #for cleaning up when the program stops. the jack server can do it in newer jack versions, but this is safer.

cjack.jack_get_uuid_for_client_name.argtypes = [ctypes.POINTER(jack_client_t), ctypes.c_char_p]
cjack.jack_get_uuid_for_client_name.restype = ctypes.c_char_p

ourJackUuid = cjack.jack_get_uuid_for_client_name(ctypesJackClient, clientName.encode("ascii"))
ourJackUuid = int(ourJackUuid.decode("UTF-8"))
ctypesJackUuid = jack_uuid_t(ourJackUuid)

cjack.jack_set_property(ctypesJackClient, ctypesJackUuid, ctypes.c_char_p(b"pid"), ctypes.c_char_p(str(getpid()).encode()), None)

##################
#Start everything
qtApp.exec_()
