#!/usr/bin/python3

import os
import argparse
import shutil
import string
import uuid
import gzip
from datetime import datetime
import signal
from time import sleep
import logging

import urllib3.exceptions
from minio import Minio


VALID_FILE_CHARS = set("-_.()%s%s" % (string.ascii_letters, string.digits))


def make_segment_filename(s, is_short_ids):
    r = ""
    for i in s:
        if i == "/":
            r += "."
        elif i in VALID_FILE_CHARS:
            r += i
        else:
            r += "x"
    if not is_short_ids:
        r += ".signed_uids"
    return r


def get_sleep_subperiods(t):
    v = t / 0.1
    for i in range(int(v)):
        yield 0.1
    yield 0.1 * (v - int(v))


class Application:
    def __init__(self):
        self.plugins = {
            ".gz": gzip.open
        }

        self.running = True
        signal.signal(signal.SIGUSR1, self.__stop)

        parser = argparse.ArgumentParser()
        parser.add_argument("--url", help="URL of server.")
        parser.add_argument("--acc", help="Access key (login).")
        parser.add_argument("--secret", help="Secret key (password).")
        parser.add_argument("--bucket", help="Bucket name.")
        parser.add_argument("--out-dir", help="Directory to download files to.")
        parser.add_argument("--marker-dir", help="Directory to save already processed files.")
        parser.add_argument("--period", type=float, help="Period between attempts.")
        parser.add_argument("--tmp-dir", default="/tmp", help="Temp directory.")
        parser.add_argument("--pid-file", required=False, help="File with process ID.")
        args = parser.parse_args()
        self.url = args.url
        self.acc = args.acc
        self.secret = args.secret
        self.bucket = args.bucket
        self.out_dir = args.out_dir
        os.makedirs(self.out_dir, exist_ok=True)
        self.marker_dir = args.marker_dir
        os.makedirs(self.marker_dir, exist_ok=True)
        self.period = args.period
        self.tmp_dir = args.tmp_dir
        os.makedirs(self.tmp_dir, exist_ok=True)
        self.pid_file = args.pid_file

    def run(self):
        try:
            self.on_start()
        finally:
            self.on_stop()

    def on_start(self):
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))

        while self.running:
            meta = None
            client = Minio(self.url, access_key=self.acc, secret_key=self.secret)
            objects = client.list_objects(self.bucket)
            output_files = {}
            markers = []
            is_error = True
            try:
                for obj in objects:
                    if not self.running:
                        break
                    name = obj.object_name
                    if name == "meta.tsv":
                        continue
                    marker_path = os.path.join(self.marker_dir, name)
                    if os.path.exists(marker_path):
                        continue
                    markers.append(marker_path)
                    if meta is None:
                        meta = self.__load_meta(client)
                    file_type = open
                    for k, v in self.plugins.items():
                        if name.endswith(k):
                            file_type = v
                            break
                    print("Downloading " + name)
                    response = client.get_object(self.bucket, name)
                    try:
                        tmp_path = os.path.join(self.tmp_dir, name)
                        try:
                            with open(tmp_path, "wb") as f:
                                for batch in response.stream():
                                    if not self.running:
                                        break
                                    f.write(batch)
                            print("Processing " + name)
                            with file_type(tmp_path, "r") as f:
                                while True:
                                    if not self.running:
                                        break
                                    line = f.readline().decode("utf-8")
                                    if not line:
                                        break
                                    if line.endswith("\n"):
                                        line = line[:-1]
                                    user_id, segment_ids = line.split("\t")
                                    is_short = len(user_id) < 32
                                    for segment_id in segment_ids.split(","):
                                        file_key = (segment_id, is_short)
                                        first_line = False
                                        try:
                                            output_file = output_files[file_key]
                                        except KeyError:
                                            first_line = True
                                            use_segment_id = (meta[segment_id] if segment_id in meta else str(segment_id))
                                            output_file = open(
                                                os.path.join(self.tmp_dir, make_segment_filename(use_segment_id, is_short)),
                                                "wt")
                                            output_files[file_key] = output_file
                                        if not first_line:
                                            output_file.write("\n")
                                        output_file.write(user_id)
                        finally:
                            self.__safe_remove(tmp_path)
                    finally:
                        response.close()
                        response.release_conn()
            except urllib3.exceptions.MaxRetryError:
                logging.error("Minio error")
            else:
                is_error = False
            finally:
                stamp = None
                if self.running:
                    stamp = ".stamp_" + datetime.now().strftime("%Y_%m_%d_%H_%M_%S_") + str(uuid.uuid4())
                    if not is_error:
                        for marker_path in markers:
                            with open(marker_path, "w"):
                                pass
                for k, f in output_files.items():
                    segment_id, is_short = k
                    f.close()
                    use_segment_id = (meta[segment_id] if segment_id in meta else str(segment_id))
                    fname = make_segment_filename(use_segment_id, is_short)
                    tmp_path = os.path.join(self.tmp_dir, fname)
                    if stamp is None or is_error:
                        self.__safe_remove(tmp_path)
                    else:
                        shutil.move(tmp_path, os.path.join(self.out_dir, fname + stamp))
            for t in get_sleep_subperiods(self.period):
                if not self.running:
                    return
                sleep(t)

    def on_stop(self):
        if self.pid_file is not None:
            os.remove(self.pid_file)

    def __load_meta(self, client):
        meta = {}
        for line in client.get_object(self.bucket, "meta.tsv").data.decode("utf-8").split("\n"):
            if line:
                meta_k, meta_v = line.split("\t")
                meta[meta_k] = meta_v
        return meta

    def __stop(self, signum, frame):
        print("Stop signal received");
        self.running = False

    def __safe_remove(self, path):
        if os.path.isfile(path):
            os.remove(path)


def main():
    app = Application()
    app.run()


if __name__ == "__main__":
    main()

