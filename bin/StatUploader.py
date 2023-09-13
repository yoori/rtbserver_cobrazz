#!/usr/bin/python3

import os
import subprocess
import shutil
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context


class Upload:
    def __init__(self, service, params, type_name):
        self.service = service
        self.in_dir = params["in_dir"]
        self.failure_dir = params.get("failure_dir")
        if self.failure_dir is not None:
            os.makedirs(self.failure_dir, exist_ok=True)
        self.type_name = type_name

    def process(self):
        with Context(self.service, in_dir=self.in_dir) as ctx:
            for in_name in ctx.files.get_in_names():
                self.service.print_(0, f"Processing {in_name}")
                in_path = os.path.join(ctx.in_dir, in_name)
                error = self.process_file(in_path)
                if error is None or self.failure_dir is None:
                    self.service.print_(0, f"Removing {in_name}")
                    os.remove(in_path)
                else:
                    self.service.print_(0, f"Failure in {in_name}:\n{error}")
                    shutil.move(in_path, self.failure_dir)

    def process_file(self, in_path):
        raise NotImplementedError


class ClickhouseUpload(Upload):
    def process_file(self, in_path):
        cmd = f'cat "{in_path}" | clickhouse-client -h "{self.service.ch_host}" --query="INSERT INTO {self.type_name} FORMAT CSV"'
        with subprocess.Popen(['sh', '-c', cmd], stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
            stdout, stderr = proc.communicate()
            return None if proc.returncode == 0 else stderr.decode()


UPLOAD_TYPES = {}


def add_upload_type(base_type, type_name):
    if type_name in UPLOAD_TYPES:
        raise RuntimeError(f"Upload type {type_name} already registered")

    class NewUpload(base_type):
        def __init__(self, service, params):
            super().__init__(service, params, type_name)

    UPLOAD_TYPES[type_name] = NewUpload


add_upload_type(ClickhouseUpload, "RequestStatsHourlyExtStat")


class Application(Service):
    def __init__(self):
        super().__init__()
        self.args_parser.add_argument("--ch-host", help="Clickhouse hostname.")
        self.args_parser.add_argument("--upload-type", help="Upload type (for single upload).")
        self.args_parser.add_argument("--failure-dir", help="Directory that stores failure files (for single upload).")
        self.__uploads = []

    def on_start(self):
        super().on_start()

        self.ch_host = self.params["ch_host"]

        def add_upload(params):
            type_name = params["upload_type"]
            try:
                upload_type = UPLOAD_TYPES[type_name]
            except KeyError:
                raise RuntimeError(f"Upload type {type_name} not found")
            self.__uploads.append(upload_type(self, params))

        if self.params.get("upload_type") is not None:
            add_upload(self.params)
        for upload in self.config.get("uploads", tuple()):
            add_upload(upload)

    def on_timer(self):
        for upload in self.__uploads:
            upload.process()


if __name__ == "__main__":
    service = Application()
    service.run()

