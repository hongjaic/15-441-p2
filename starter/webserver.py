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
import flask

app = Flask(__name__)

DEBUG = True

app.config.from_object(__name__)

localhost = socket.gethostbyname(socket.gethostname())
if localhost == '127.0.0.1':
    import commands
    output = commands.getoutput('/sbin/ifconfig')
    localhost = parseaddress(output)

conffile = open('node1.conf')
conffirstline = conffile.readline()
localconf = conffirstline.split()

locport = int(localconf[3])
servport = localconf[4]

conffile.close()
@app.route('/')
def index():
	return redirect(url_for('static', filename='index.html'))

@app.route('/r',methods=["GET"])

def get_redir():


    objname = request.args.get('object')

    return redirect(url_for('static', filename='f/'+objname))


@app.route('/static/f/<obj>')
def rd_getrd(obj):

    objname = str(obj)
    objlen = str(len(objname))
    msg = 'RDGET '+objlen+' '+objname

    rdsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    rdsock.connect((localhost,locport))
    print 'connected'
    rdsock.send(msg)
    print 'sent '+msg
    rdresponse = rdsock.recv(4096)
    print 'recv '+rdresponse
    rdsock.close()

    responsewords = rdresponse.split(' ')

    status = responsewords[0]
    url_len = responsewords[1]
    url = responsewords[2][:-1]

    if status == 'OK':
        the_file = urllib.urlopen(url)
        resp = flask.send_file(the_file)
    elif status == 'NOTFOUND':
        resp = flask.make_response(flask.render_template('404.html'), 404)
    else:
        resp = flask.make_response(flask.render_template('500InternalServerError.html'), 500)

    return resp


@app.route('/r',methods=["POST"])
def rd_addfile():
    curr_root = os.getcwd()

    objname = request.form['object']

    file = request.files['uploadFile']

    tmpname = tempfile.mktemp(prefix=curr_root + '/static/')

    file.save(tmpname)

    hash = hashlib.sha256()

    f = open(tmpname, 'r')

    while True:
        fdata = f.read(4096)

        if len(fdata) == 0:
            break

        hash.update(fdata)

    finalname = '/static/' + hash.hexdigest()
    shutil.move(tmpname, curr_root + finalname)

    f.close()

    rdsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    rdsock.connect((localhost, locport))
    obj = str(objname)
    objlen = str(len(obj))
    final = str(finalname)
    finallen = str(len(final))
    rdsock.send('ADDFILE ' + objlen +' '+obj+' '+final+' '+finallen)

    rdresponse = rdsock.recv(4096)
    rdsock.close()

    responsewords = rdresponse.split(' ')

    if responsewords[0] == 'OK':
        resp = redirect(url_for('static', filename='index.html'))
    else:
        resp = flask.make_response(flask.render_template('500InternalServerError.html'), 500)

    return resp


def generate(url):
    fp = urllib.urlopen(url)

    while True:
        strm = fp.read(4096)

        if len(strm) == 0:
            break

        yield strm

    fp.close()


if __name__ == '__main__':
	if (len(sys.argv) > 0):
		app.run(host='0.0.0.0', port=int(servport), threaded=True, processes=1)
	else:
		print "Usage ./webserver\n"

