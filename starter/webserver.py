#!/usr/bin/env python

from __future__ import with_statement
ALLDIRS = ['/afs/andrew.cmu.edu/usr21/hongjaic/flask_env/lib/python2.6/site-packages']

"""webserver.py"""

__author__ = 'Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>'

#from flask import Flask, redirect, url_for, request
import sys
import os
import hashlib
import socket

import site
import urllib
import hashlib
import shutil
import tempfile

# Remember original sys.path.
prev_sys_path = list(sys.path) 

# Add each new site-packages directory.
for directory in ALLDIRS:
    site.addsitedir(directory)

# Reorder sys.path so new directories at the front.
new_sys_path = [] 
for item in list(sys.path): 
    if item not in prev_sys_path: 
        new_sys_path.append(item) 
        sys.path.remove(item) 
        sys.path[:0] = new_sys_path 


from flask import Flask, redirect, url_for, request

app = Flask(__name__)

DEBUG = True

app.config.from_object(__name__)

localhost = socket.gethostbyname(socket.gethostname())
if localhost == '127.0.0.1':
    import commands
    output = commands.getoutput('/sbin/ifconfig')
    localhost = parseaddress(output)

@app.route('/')
def index():
	return redirect(url_for('static', filename='index.html'))

@app.route('/rd/<int:p>', methods=["GET"])
def rd_getrd(p):
    objname = request.forms['object']

    rdsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    rdsock.connect((localhost, p))

    rdsock.send('RDGET ' + objname)
    rdresponse = rdclientsock.recv(4096)
    rdsock.close()

    responsewords = string.split(rdresponse)

    if responsewords[0] == 'OK':
        rdurl = responsewords[1]
        return Response(generate(rdurl))
    elif '404' in rdresponse:
        return render_template('./static/404NotFound.html'), 404
    
    return render_template('./static/index.html')

@app.route('/rd/addfile/<int:p>', methods=["POST"])
def rd_addfile(p):
    objname = request.fomrs['object']
    file = request.files['uploadFile']

    tmpname = tempfile.mktemp(prefix='./static/')
    file.save(tmpname)

    hash = hashlib.sha256()

    f = file.open(tmpname)

    while True:
        fdata = f.read(4096)

        if len(fdata) == 0:
            break

        hash.upate(fdata)

    finalname = './static/' + hash.hexdigest()
    shutil.move(tmpname, finalname)

    rdsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    rdsock.connect((localhost, p))

    rdsock.send('ADDFILE ' + finalname)
    rdresponse = rdclientsock.recv(4096)
    rdsock.close()

    if 'OK' in rdresponse:
        return render_template('./static/index.html')

    return render_template('./static/index.html')


@app.route('/rd/<int:p>/<obj>', methods=["GET"])
def rd_getrdpeer(p, obj):
    rdsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    rdsock.connect((localhost, p))

    rdsock.send('RDGET ' + obj)
    rdresponse = rdclientsock.recv(4096)
    rdsock.close()

    responsewords = string.split(rdresponse)

    if responsewords[0] == 'OK':
        rdurl = responsewords[1]
        return Response(generate(rdurl))
    elif '404' in rdresponse:
        return render_template('./static/404NotFound.html'), 404
    
    return render_template('./static/index.html')


def generate(url):
    fp = urllib.urlopen(url)
    
    while True:
        strm = fp.read(4096)
        
        if len(strm) == 0:
            break
        
        yield strm
        
    fp.close()

if __name__ == '__main__':
	if (len(sys.argv) > 1):
		servport = int(sys.argv[1])
		app.run(host='0.0.0.0', port=servport, threaded=True, processes=1)
	else:	
		print "Usage ./webserver <server-port> \n"
