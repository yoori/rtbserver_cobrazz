#!/usr/bin/python3

import os
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context


class Upload:
    def __init__(self, service, params, table_name):
        self.service = service
        self.__in_dir = params["in_dir"]
        self.__table_name = table_name

    def process(self):
        with Context(self.service, in_dir=self.__in_dir) as ctx:
            for in_file in ctx.files.get_in_files():
                self.service.print_(0, f"Processing {in_file}")
                in_path = os.path.join(ctx.in_dir, in_file)
                exit_status = os.system(
                    f'cat "{in_path}" | clickhouse-client -h "{self.service.ch_host}" --query="INSERT INTO {self.__table_name} FORMAT CSV"')
                if exit_status != 0:
                    raise RuntimeError(f"Bad clickhouse-client exit status {exit_status}")
                self.service.print_(0, f"Removing {in_file}")
                os.remove(in_path)


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

