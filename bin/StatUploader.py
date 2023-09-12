#!/usr/bin/python3

import os
import re
from datetime import datetime
import clickhouse_driver
import clickhouse_driver.errors
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context
from ServiceUtilsPy.LineIO import LineReader


NULLABLE_RE = re.compile(r"Nullable\((.*)\)")
INT_RE = re.compile(r"Int(.*)")
UINT_RE = re.compile(r"UInt(.*)")
FIXED_STRING_RE = re.compile(r"FixedString\((.*)\)")
DECIMAL_RE = re.compile(r"Decimal\((.*),(.*)\)")


def bool_to_value(s):
    return bool(int(s))


def date_time_to_value(s):
    return datetime.strptime(s, "%Y-%m-%d %H:%M:%S")


def make_nullable_to_value(f):

    def nullable_to_value(s):
        return f(s) if s else None

    return nullable_to_value


def get_column_converter(column_type):
    if column_type == "Bool":
        return bool_to_value

    if column_type == "DateTime":
        return date_time_to_value

    match = NULLABLE_RE.match(column_type)
    if match is not None:
        return make_nullable_to_value(get_column_converter(match.groups()[0]))

    match = INT_RE.match(column_type)
    if match is not None:
        return int

    match = UINT_RE.match(column_type)
    if match is not None:
        return int

    match = FIXED_STRING_RE.match(column_type)
    if match is not None:
        return str

    match = DECIMAL_RE.match(column_type)
    if match is not None:
        return str

    return None


class Upload:
    def __init__(self, service, params, table_name):
        self.service = service
        self.__in_dir = params["in_dir"]
        self.__batch_size = params.get("batch_size", service.batch_size)
        self.__table_name = table_name
        self.__describe_table_sql = f"DESCRIBE TABLE {table_name}"

    def process(self):
        ch_column_types = dict(
            (column_name, column_type)
            for column_name, column_type, _, _, _, _, _ in self.service.ch_client.execute(self.__describe_table_sql))
        values = []
        with Context(self.service, in_dir=self.__in_dir) as ctx:
            for in_file in ctx.files.get_in_files():
                self.service.print_(0, f"Processing {in_file}")
                in_path = os.path.join(ctx.in_dir, in_file)
                with LineReader(self.service, path=in_path) as f:
                    column_converters = []
                    csv_header = f.read_line(progress=False)
                    insert_sql = f"INSERT INTO {self.__table_name}({csv_header}) VALUES"

                    def flush():
                        if values:
                            self.service.ch_client.execute(insert_sql, values)
                            values.clear()

                    for csv_column_name in csv_header.split(","):
                        try:
                            ch_column_type = ch_column_types[csv_column_name]
                        except KeyError:
                            raise RuntimeError(f"{self.__table_name} column '{csv_column_name}' not found")
                        column_converter = get_column_converter(ch_column_type)
                        if column_converter is None:
                            raise RuntimeError(f"{self.__table_name} column type '{ch_column_type}' not found")
                        column_converters.append(column_converter)
                    for line in f.read_lines():
                        columns = line.split(",")
                        if len(columns) != len(column_converters):
                            raise RuntimeError(f"{self.__table_name} CSV column count mismatch")
                        values.append(list(
                            column_converters[i](columns[i])
                            for i in range(len(column_converters))
                        ))
                        if len(values) >= self.__batch_size:
                            flush()
                    flush()
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
        self.args_parser.add_argument("--batch-size", type=int, default=100, help="SQL batch size (for single upload).")
        self.args_parser.add_argument("--upload-type", help="Upload type (for single upload).")
        self.ch_client = None
        self.__uploads = []

    def on_start(self):
        super().on_start()

        self.batch_size = self.params["batch_size"]

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
        try:
            if self.ch_client is None:
                self.ch_client = clickhouse_driver.Client(host=self.params["ch_host"])
            for upload in self.__uploads:
                upload.process()
        except clickhouse_driver.errors.Error as e:
            self.print_(0, e)
            self.__close_ch_client()

    def on_stop(self):
        self.__close_ch_client()
        super().on_stop()

    def __close_ch_client(self):
        if self.ch_client is not None:
            self.ch_client.disconnect()
            self.ch_client = None


if __name__ == "__main__":
    service = Application()
    service.run()

