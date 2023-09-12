#!/usr/bin/python3

import os
import subprocess
import shutil
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context


class Upload:
    def __init__(self, service, params, table_name):
        self.service = service
        self.__in_dir = params["in_dir"]
        self.__failure_dir = params.get("failure_dir")
        if self.__failure_dir is not None:
            os.makedirs(self.__failure_dir, exist_ok=True)
        self.__table_name = table_name

    def process(self):
        with Context(self.service, in_dir=self.__in_dir) as ctx:
            for in_file in ctx.files.get_in_files():
                self.service.print_(0, f"Processing {in_file}")
                in_path = os.path.join(ctx.in_dir, in_file)
                cmd = f'cat "{in_path}" | clickhouse-client -h "{self.service.ch_host}" --query="INSERT INTO {self.__table_name} FORMAT CSV"'
                with subprocess.Popen(['sh', '-c', cmd], stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
                    stdout, stderr = proc.communicate()
                    if proc.returncode == 0 or self.__failure_dir is None:
                        self.service.print_(0, f"Removing {in_file}")
                        os.remove(in_path)
                    else:
                        self.service.print_(0, f"Failure in {in_file}:\n{stderr.decode()}")
                        shutil.move(in_path, self.__failure_dir)


class RequestStatsHourlyExtStatUpload(Upload):
    def __init__(self, service, params):
        super().__init__(service, params, "RequestStatsHourlyExtStat")


UPLOAD_TYPES = {
    "RequestStatsHourlyExtStat": RequestStatsHourlyExtStatUpload
}


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
            upload_type_name = params["upload_type"]
            try:
                upload_type = UPLOAD_TYPES[upload_type_name]
            except KeyError:
                raise RuntimeError(f"Upload type {upload_type_name} not found")
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

