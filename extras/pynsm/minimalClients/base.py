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

#/nsm/server/announce  #A hack to get this into Agordejo launcher discovery


from nsmclient import NSMClient
import sys
from time import sleep
import os
from threading import Timer

sys.path.append(os.getcwd())

class BaseClient(object):

    def saveCallbackFunction(self, ourPath, sessionName, ourClientNameUnderNSM):
        print (__file__, "save")

    def openOrNewCallbackFunction(self, ourPath, sessionName, ourClientNameUnderNSM):
        print (__file__,"open/new")

    def exitCallbackFunction(self, ourPath, sessionName, ourClientNameUnderNSM):
        print (__file__, "quit")
        sys.exit()

    def broadcastCallbackFunction(self, ourPath, sessionName, ourClientNameUnderNSM, messagePath, listOfArguments):
        print (__file__, "broadcast")

    def event(self, nsmClient):
        pass

    def __init__(self, name, delayedFunctions=[], eventFunction=None):
        """delayedFunctions are a (timer delay in seconds, function call) list of tuples. They will
        be executed once.
        If the function is a string instead it will be evaluated in the BaseClient context,
        providing self. Do not give a lambda!


        Give eventFunction for repeated execution."""

        self.nsmClient = NSMClient(prettyName = name, #will raise an error and exit if this example is not run from NSM.
            saveCallback = self.saveCallbackFunction,
            openOrNewCallback = self.openOrNewCallbackFunction,
            supportsSaveStatus = False,         # Change this to True if your program announces it's save status to NSM
            exitProgramCallback = self.exitCallbackFunction,
            broadcastCallback = self.broadcastCallbackFunction,
            hideGUICallback = None, #replace with your hiding function. You need to answer in your function with nsmClient.announceGuiVisibility(False)
            showGUICallback = None,  #replace with your showing function. You need to answer in your function with nsmClient.announceGuiVisibility(True)
            loggingLevel = "info", #"info" for development or debugging, "error" for production. default is error.
            )

        if eventFunction:
            self.event = eventFunction

        for delay, func in delayedFunctions:
            if type(func) is str:
                func = eval('lambda self=self: ' + func )
            t = Timer(interval=delay, function=func, args=())
            t.start()

        while True:
            self.nsmClient.reactToMessage()
            self.event(self.nsmClient)
            sleep(0.05)

if __name__ == '__main__':
    """This is the most minimal nsm client in existence"""
    BaseClient(name="testclient_base") #this never returns an object.

