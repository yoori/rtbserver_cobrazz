#! /usr/bin/env python3

import csv
import os
import pathlib
import shutil
import signal
import sys
import threading
import time
from unittest.mock import patch
sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from SegmentUploader import Application  # noqa: E402


class HTTPResponseMock():
    def __init__(self, url, headers, params, ssl, status, request_info,
                 history):
        self.url = url
        self.headers = headers
        self.params = params
        self.ssl = ssl
        self.status = status
        self.request_info = request_info
        self.history = history

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_value, exc_tb):
        pass


class HTTPConnectionMock():
    data = {}
    lock = threading.Lock()
    errors = []

    def init():
        with HTTPConnectionMock.lock:
            with open("uploader/data/requests.csv", newline='',
                      encoding='utf-8-sig') as cf:
                reader = csv.DictReader(cf, delimiter=';')
                index = 0
                for line in reader:
                    index += 1
                    key = line["url"] + line["headers"] + line["params"]
                    HTTPConnectionMock.data[key] = 0

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_value, exc_tb):
        pass

    def get(self, url, headers, params, ssl=False):
        with HTTPConnectionMock.lock:
            key = url + str(headers) + str(params)
            HTTPConnectionMock.data[key] = (
                HTTPConnectionMock.data.get(key, 0) + 1)
            return HTTPResponseMock(url, headers, params, ssl, 204, "", "")

    def check():
        with HTTPConnectionMock.lock:
            for line in HTTPConnectionMock.data:
                value = HTTPConnectionMock.data[line]
                if value != 1:
                    HTTPConnectionMock.errors.append(
                            f"Expected 1. Actual {value}. Info {line}.")


async def test_one():
    cwd = pathlib.Path(__file__).parent.resolve()
    os.chdir(cwd)

    work_path = os.path.join(cwd, "uploader")

    if not os.path.exists(work_path):
        os.mkdir(work_path)
    if os.path.exists(os.path.join(work_path, "data")):
        shutil.rmtree(os.path.join(work_path, "data"))
    shutil.copytree(os.path.join(cwd, "data"),
                    os.path.join(work_path, "data"))

    testargs = ["prog", "--config", "uploader/data/config.json"]

    with patch.object(sys, 'argv', testargs):
        service = Application()
        HTTPConnectionMock.init()
        service.HTTPConnection = HTTPConnectionMock

        def check_end(service) -> bool:
            if hasattr(service, 'params'):
                for dir_info in service.params["in_dirs"]:
                    directory = dir_info["in_dir"]
                    for _, _, fnames in os.walk(directory):
                        for f in fnames:
                            if not f.endswith("taxonomy.csv"):
                                return True
            else:
                return True
            return False

        def wait_application(service):
            wait = 0
            delta = 0.01
            while check_end(service) and wait < 10:
                wait += delta
                time.sleep(delta)
            service.HTTPConnection.check()
            os.kill(os.getpid(), signal.SIGINT)

        threads = []

        thread = threading.Thread(target=service.run)
        threads.append(thread)
        thread.start()

        thread = threading.Thread(target=wait_application, args=(service,))
        threads.append(thread)
        thread.start()

        for thread in threads:
            thread.join()

        assert len(HTTPConnectionMock.errors) == 0

        if os.path.exists(work_path):
            shutil.rmtree(work_path)
