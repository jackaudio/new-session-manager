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

#Test file for the resource import function in nsmclient.py


if __name__ == "__main__":

    from nsmclient import NSMClient
    #We do not start nsmclient, just the the function with temporary files
    importResource = NSMClient.importResource
    import os, os.path

    import logging
    logging.basicConfig(level=logging.INFO)

    from inspect import currentframe
    def get_linenumber():
        cf = currentframe()
        return cf.f_back.f_lineno

    from tempfile import mkdtemp
    class self(object):
        ourPath = mkdtemp()
        ourClientNameUnderNSM = "Loader Test"
    assert os.path.isdir(self.ourPath)


    #First a meta test to see if our system is working:
    assert os.path.exists("/etc/hostname")
    try:
        result = importResource(self, "/etc/hostname") #should not fail!
    except FileNotFoundError:
        pass
    else:
        print (f"Meta Test System works as of line {get_linenumber()}")
        print ("""You should not see any "Test Error" messages""")
        print ("Working in", self.ourPath)
        print (f"Removing {result} for a clean test environment")
        os.remove(result)
        print()

    #Real tests

    try:
        importResource(self, "/floot/nonexistent") #should fail
    except FileNotFoundError:
        pass
    else:
        print (f"Test Error in line {get_linenumber()}")

    try:
        importResource(self, "////floot//nonexistent/") #should fail
    except FileNotFoundError:
        pass
    else:
        print (f"Test Error in line {get_linenumber()}")

    try:
        importResource(self, "/etc/shadow") #reading not possible
    except PermissionError:
        pass
    else:
        print (f"Test Error in line {get_linenumber()}")


    assert os.path.exists("/etc/hostname")
    try:
        org = self.ourPath
        self.ourPath = "/" #writing not possible
        importResource(self, "/etc/hostname")
    except PermissionError:
        self.ourPath = org
    else:
        print (f"Test Error in line {get_linenumber()}")


    from tempfile import NamedTemporaryFile
    tmpf = NamedTemporaryFile()
    assert os.path.exists("/etc/hostname")
    try:
        org = self.ourPath
        self.ourPath = tmpf.name #writable, but not a dir
        importResource(self, "/etc/hostname")
    except NotADirectoryError:
        self.ourPath = org
    else:
        print (f"Test Error in line {get_linenumber()}")

    #Test the real purpose
    result = importResource(self, "/etc/hostname")
    print ("imported to", result)


    #Test what happens if we try to import already imported resource again
    result = importResource(self, result)
    print ("imported to", result)

    #Test what happens if we try to import a resource that would result in a name collision
    result = importResource(self, "/etc/hostname")
    print ("imported to", result)

    #Count the number of resulting files.
    assert len(os.listdir(self.ourPath)) == 2

