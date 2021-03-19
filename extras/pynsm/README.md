# pynsm
NSM/New Session Manager client library in Python - No dependencies except Python3.

PyNSMClient -  A New Session Manager Client-Library in one file.

Copyright (c) since 2014: Laborejo Software Suite <info@laborejo.org>, All rights reserved.

This library is licensed under the MIT license. Please check the file LICENSE for more information.

This library has no version numbers, or releases. It is a "rolling" git. Copy it into your source and update only if you need new features.

## Short Instructions
Before you start a word about control flow: NSM and any client, like this one, can be
considered automation and remote-control of GUI operations, normally done by the user. That means
this module needs to be included in your GUI code. The author personally creates the pynsm-object
as early as possible in his PyQt Mainwindow class.

Copy nsmclient.py to your own program and import and initialize it as early as possible (see below)
Then add nsmClient.reactToMessage to your existing event loop.

    from nsmclient import NSMClient
    nsmClient = NSMClient(prettyName = niceTitle, #will raise an error and exit if this example is not run from NSM.
        saveCallback = saveCallbackFunction,
        openOrNewCallback = openOrNewCallbackFunction,
        supportsSaveStatus = False,         # Change this to True if your program announces it's save status to NSM
        exitProgramCallback = exitCallbackFunction,
        hideGUICallback = None, #replace with your hiding function. You need to answer in your function with nsmClient.announceGuiVisibility(False)
        showGUICallback = None,  #replace with your showing function. You need to answer in your function with nsmClient.announceGuiVisibility(True)
        broadcastCallback = None,  #give a function that reacts to any broadcast by any other client.
        sessionIsLoadedCallback = None, #give a function that reacts to the one-time state when a session has fully loaded.
        loggingLevel = "info", #"info" for development or debugging, "error" for production. default is error.
        )


Don't forget to add nsmClient.reactToMessage to your event loop.

* Each of the callbacks "save", "open/new" and "exit" receive three parameters: ourPath, sessionName, ourClientNameUnderNSM.
* openOrNew gets called first. Init your jack client there with ourClientNameUnderNSM as name.
* exitProgramCallback is the place to gracefully exit your program, including jack-client closing.
* saveCallback gets called all the time. Use ourPath either as filename or as directory.
    * If you choose filename add an extension.
    * If you choose directory make sure that the filenames inside are static, no matter what project/session. The user must have no influence over file naming
* broadcastCallback receives five parameters. The three standard: ourPath, sessionName, ourClientNameUnderNSM. And additionally messagePath and listOfArguments. MessagePath is entirely program specific, the number and type of arguments depend on the sender.
* sessionIsLoadedCallback receives no parameters. It is ONLY send once, after the session is fully loaded. This is NOT the place to delay your announce. If you add your client to a running session (which must happen at some point) sessionLoaded will NOT get called.
* Additional callbacks are: hideGUICallback and showGUICallback. These receive no parameters and need to answer with the function: nsmClient.announceGuiVisibility(bool). That means you can decline show or hide, dependending on the state of your program.

The nsmClient object has methods and variables such as:

* nsmClient.ourClientNameUnderNSM
  * Always use this name for your program
* nsmClient.announceSaveStatus(False)
  * Announce your save status (dirty = False / clean = True), If your program sends those messages set supportsSaveStatus = True when intializing NSMClient with both hideGUICallback and showGUICallback
* nsmClient.sessionName
* nsmClient.ourOscUrl = osc.udp://{ip}:{port}/  Use this to broadcast your presence, to handshake communication between different programs
* nsmClient.announceGuiVisibility(bool)
  * Announce if your GUI is visible (True) or not (False). Only works if you initialized NSMClient with both hideGUICallback and showGUICallback. Don't forget to send it once for your state after starting your program.
* nsmcClient.changeLabel(prettyName)
  * Tell the GUI to append (prettyName) to our name. This is not saved by NSM but you need to send it yourself each startup.
* nsmClient.serverSendSaveToSelf()
  * A clean solution to use the nsm save callbacks from within your program (Ctrl+S or File->Save). No need for redundant save mechanism.
* nsmClient.serverSendExitToSelf()
  * A clean quit, without "client died unexpectedly". Use sys.exit() to exit your program in your nms-quit callback.
* nsmClient.importResource(filepath)
  * Use this to load external resources, for example a sample file. It links the sample file into the session dir, according to the NSM rules, and returns the path of the linked file.
* nsmClient.debugResetDataAndExit()
  * Deletes self.ourpath, which is the session save file or directory, recursively and exits the client. This is only meant for debugging and testing.

## Long Instructions
* Read and start example.py, then read and understand nsmclient.py. It requires PyQt5 to execute and a brain to read.
* There are several very minimal and basic clients in the directory `minimalClients/`
* For your own program read and learn the NSM API: http://non.tuxfamily.org/nsm/API.html
* The hard part about session management is not to use this lib or write your own but to make your program comply to the strict rules of session management.

## Additional Examples
More examples can be found in `/minimalClients`. This mimics a minimal, but functional program.
To actually run the programs and attach them to a running session execute the inluced file `source_me_with_port.bash <PORT>`
where <PORT> is the NSM port the server printed out after starting.

You can see that nsmclient.py is included in this dir again, to avoid redundancy only as symlink.
In your real program this would be the real file.

## Sources and Influences
* The New-Session-Manager by Jonathan Moore Liles <male@tuxfamily.org>: http://non.tuxfamily.org/nsm/
* New Session Manager by Nils Hilbricht et al  https://new-session-manager.jackaudio.org
* With help from code fragments from https://github.com/attwad/python-osc ( DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE v2 )
