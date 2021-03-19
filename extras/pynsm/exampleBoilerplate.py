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


from time import sleep              # main event loop at the bottom of this file
# nsm only
from nsmclient import NSMClient     # will raise an error and exit if this example is not run from NSM.

########################################################################
# General
########################################################################
niceTitle = "myNSMprogram"            # This is the name of your program. This will display in NSM and can be used in your save file

########################################################################
# Prepare the NSM Client
# This has to be done as the first thing because NSM needs to get the paths
# and may quit if NSM environment var was not found.
#
# This is also where to set up your functions that react to messages from NSM,
########################################################################
# Here are some variables you can use:
# ourPath               = Full path to your NSM save file/directory with serialized NSM extension
# ourClientNameUnderNSM = Your NSM save file/directory with serialized NSM extension and no path information
# sessionName           = The name of your session with no path information
########################################################################

def saveCallback(ourPath, sessionName, ourClientNameUnderNSM):
    # Put your code to save your config file in here
    print("saveCallback");


def openCallback(ourPath, sessionName, ourClientNameUnderNSM):
    # Put your code to open your config file in here
    print("openCallback");


def exitProgram(ourPath, sessionName, ourClientNameUnderNSM):
    """This function is a callback for NSM.
    We have a chance to close our clients and open connections here.
    If not nsmclient will just kill us no matter what
    """
    print("exitProgram");
    # Exit is done by NSM kill.


def showGUICallback():
    # Put your code that shows your GUI in here
    print("showGUICallback");
    nsmClient.announceGuiVisibility(isVisible=True)  # Inform NSM that the GUI is now visible. Put this at the end.


def hideGUICallback():
    # Put your code that hides your GUI in here
    print("hideGUICallback");
    nsmClient.announceGuiVisibility(isVisible=False)  # Inform NSM that the GUI is now hidden. Put this at the end.


nsmClient = NSMClient(prettyName = niceTitle,
                      saveCallback = saveCallback,
                      openOrNewCallback = openCallback,
                      showGUICallback = showGUICallback,  # Comment this line out if your program does not have an optional GUI
                      hideGUICallback = hideGUICallback,  # Comment this line out if your program does not have an optional GUI
                      supportsSaveStatus = False,         # Change this to True if your program announces it's save status to NSM
                      exitProgramCallback = exitProgram,
                      loggingLevel = "info", # "info" for development or debugging, "error" for production. default is error.
                      )

# If NSM did not start up properly the program quits here.

########################################################################
# If your project uses JACK, activate your client here
# You can use jackClientName or create your own
########################################################################
jackClientName = nsmClient.ourClientNameUnderNSM

########################################################################
# Start main program loop.
########################################################################

# showGUICallback()  # If you want your GUI to be shown by default, uncomment this line
print("Entering main loop")

while True:
    nsmClient.reactToMessage()  # Make sure this exists somewhere in your main loop
    # nsmClient.announceSaveStatus(False) # Announce your save status (dirty = False / clean = True)
    sleep(0.05)
