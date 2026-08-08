#!/usr/bin/env python3
from flask import Flask, send_from_directory, request
from datetime import datetime
from types import SimpleNamespace
from io import StringIO
import random

app = Flask(__name__)
state = SimpleNamespace()
logfile = StringIO()


def init_state():
    state.wifiMode = "STA"
    state.wifiSsid = "NameTagNet"
    state.wifiPassword = ""
    state.wifiHostname = "NameTagOS"


def main():
    log("server started")
    init_state()
    app.run(debug=True)


def log(*text):
    print("{}:".format(format_time(datetime.now())), *text, file=logfile)
    print("{}:".format(format_time(datetime.now())), *text)


def format_time(dt):
    return dt.strftime("%Y-%m-%d %H:%M:%S")


@app.route("/")
@app.route("/index.html")
def index():
    return send_from_directory('.', 'index.html')


@app.route("/hyperapp.js")
def js():
    return send_from_directory('.', 'hyperapp.js')


@app.route("/file/events.log")
def events_log():
    return "Server started"


def server_info():
    return {
        "wifiMode": state.wifiMode,
        "wifiSsid": state.wifiSsid,
        "wifiHostname": state.wifiHostname,
    }


@app.route("/api/status")
def api_status():
    return server_info()


@app.route("/api/server/reboot")
def api_server_reboot():
    log("Reboot")
    return {}


@app.route("/prefs", methods=['POST'])
def prefs():
    data = request.form
    log("updating prefs ", data)

    wifiMode = data.get("wifiMode")
    if wifiMode:
        state.wifiMode = wifiMode
        log("wifiMode =", wifiMode)

    wifiSsid = data.get("wifiSsid")
    if wifiSsid:
        state.wifiSsid = wifiSsid
        log("wifiSsid =", wifiSsid)

    wifiPassword = data.get("wifiPassword")
    if wifiPassword:
        state.wifiPassword = wifiPassword
        log("wifiPassword =", wifiPassword)

    wifiHostname = data.get("wifiHostname")
    if wifiHostname:
        state.wifiHostname = wifiHostname
        log("wifiHostname =", wifiHostname)

    return server_info()


@app.route("/update", methods=['POST'])
def upload():
    data = request.form
    log("upload...", data)
    messages = ['Done!', 'Internal failure!']
    code = random.choice(range(len(messages)))
    return {
        "code": code,
        "message": messages[code],
    }


if __name__ == '__main__':
    main()
