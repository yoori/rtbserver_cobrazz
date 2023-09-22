#! /usr/bin/env python3

import os
import string
import requests
import threading
from minio import Minio
from lxml import etree
from datetime import datetime
from dateutil.relativedelta import relativedelta
from ServiceUtilsPy.File import File
from ServiceUtilsPy.LineIO import LineReader
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context
from ServiceUtilsPy.Minio import MinioRequest


VALID_FILE_CHARS = set("-_.()%s%s" % (string.ascii_letters, string.digits))


def make_segment_filename(s, is_short):
    r = ""
    for i in s:
        if i == "/":
            r += "."
        elif i in VALID_FILE_CHARS:
            r += i
        else:
            r += "x"
    if not is_short:
        r += ".signed_uids"
    return r


class Taxonomy(dict):
    def __init__(self, lines):
        super().__init__()
        for line in lines:
            if line: # check for non LineReader
                segment_id, segment = line.split("\t")
                if segment:
                    self[segment_id] = segment

    @staticmethod
    def from_file(service, path):
        with LineReader(service, path) as f:
            return Taxonomy(f.read_lines())


class Source:
    def __init__(self, service, params):
        self.service = service
        self.markers_dir = params["markers_dir"]
        self.tmp_dir = params["tmp_dir"]
        self.out_dir = params["out_dir"]

    def create_context(self, *args, **kw):
        return Context(*args, service=self.service, markers_dir=self.markers_dir, tmp_dir=self.tmp_dir, out_dir=self.out_dir, **kw)

    def process_file(self, ctx, taxonomy, in_path, name):
        self.service.print_(1, f"Processing {name}")
        with LineReader(self.service, in_path) as line_reader:
            for line in line_reader.read_lines():
                user_id, segment_ids = line.split("\t")
                is_short = len(user_id) < 32
                for segment_id in segment_ids.split(","):
                    segment_id = taxonomy.get(segment_id, segment_id)
                    output_writer = ctx.files.get_line_writer(
                        key=(segment_id, is_short),
                        name=lambda: make_segment_filename(segment_id, is_short) + ctx.fname_stamp)
                    if output_writer.first:
                        output_writer.progress.verbosity = 3
                    output_writer.write_line(user_id)


class AutoRemovePluginlessWriter(File):
    def __init__(self, ctx, name):
        super().__init__(ctx.service, os.path.join(ctx.tmp_dir, name), "wb", remove_on_exit=True, use_plugins=False)

    def write_parts(self, parts):
        for part in parts:
            self.write(part)


class AmberSource(Source):
    TAXONOMY_NAME = "meta.tsv"

    def __init__(self, service, params, type_params):
        super().__init__(service, params)
        self.__url = type_params["url"]
        self.__acc = type_params["acc"]
        self.__secret = type_params["secret"]
        self.__bucket = type_params["bucket"]
        self.__max_days = params["max_days"]

    def process(self):
        client = Minio(self.__url, access_key=self.__acc, secret_key=self.__secret)
        objects = client.list_objects(self.__bucket)
        try:
            for name, last_modified in sorted((obj.object_name, obj.last_modified) for obj in objects):
                self.service.verify_running()
                if name == self.TAXONOMY_NAME:
                    continue
                if last_modified.replace(tzinfo=None) < datetime.now() + relativedelta(days=-self.__max_days):
                    self.service.print_(1, f"Too old - {name}")
                    continue
                self.service.print_(1, f"{name}")
                with self.create_context() as ctx:
                    if ctx.markers.add(name):
                        with MinioRequest(client, self.__bucket, self.TAXONOMY_NAME) as mr:
                            taxonomy = Taxonomy(mr.data.decode("utf-8").split("\n"))
                        with MinioRequest(client, self.__bucket, name) as mr:
                            with AutoRemovePluginlessWriter(ctx, name) as w:
                                self.service.print_(1, f"Downloading {name}")
                                w.write_parts(mr.stream())
                                w.close()
                                self.process_file(ctx, taxonomy, w.path, name)
        except Exception as e:
            self.service.print_(0, e)


class AdriverSource(Source):
    def __init__(self, service, params, type_params):
        super().__init__(service, params)
        self.__url = f'https://{type_params["user"]}:{type_params["password"]}@{type_params["url"]}'
        self.__taxonomy = Taxonomy.from_file(self.service, type_params["taxonomy_file"])

    def process(self):
        try:
            with requests.get(self.__url) as files_response:
                if files_response.status_code != 200:
                    raise requests.exceptions.RequestException
                tree = etree.HTML(files_response.text)
                for name in tree.xpath("/html/body/pre/a/text()"):
                    if name == "../":
                        continue
                    with self.create_context() as ctx:
                        if ctx.markers.add(name):
                            with requests.get(f"{self.__url}/{name}", stream=True) as file_response:
                                if file_response.status_code != 200:
                                    raise requests.exceptions.RequestException
                                with AutoRemovePluginlessWriter(ctx, name) as w:
                                    self.service.print_(1, f"Downloading {name}")
                                    w.write_parts(file_response.iter_content(chunk_size=65536))
                                    w.close()
                                    self.process_file(ctx, self.__taxonomy, w.path, name)
        except Exception as e:
            self.service.print_(0, e)


class AdriverLocalSource(Source):
    def __init__(self, service, params, type_params):
        super().__init__(service, params)
        self.__dir = type_params["dir"]
        os.makedirs(self.__dir, exist_ok=True)
        self.__taxonomy = Taxonomy.from_file(self.service, type_params["taxonomy_file"])

    def process(self):
        try:
            for root, dirs, files in os.walk(self.__dir, True):
                for name in sorted(files):
                    path = os.path.join(self.__dir, name)
                    self.service.verify_running()
                    with self.create_context() as ctx:
                        if ctx.markers.add(name):
                            self.process_file(ctx, self.__taxonomy, path, name)
                    self.service.print_(1, f"Removing {path}")
                    os.remove(path)
                break
        except Exception as e:
            self.service.print_(0, e)


SOURCE_TYPES = {
    "amber": AmberSource,
    "adriver": AdriverSource,
    "adriver_local": AdriverLocalSource
}


class Application(Service):
    def on_start(self):
        super().on_start()
        self.__sources = []
        for source in self.config.get("sources", tuple()):
            added = False
            for source_type_name, source_type in SOURCE_TYPES.items():
                type_params = source.get(source_type_name)
                if type_params is not None:
                    self.__sources.append(source_type(self, source, type_params))
                    added = True
                    break
            if not added:
                raise RuntimeError("unknown source type")

    def on_timer(self):
        threads = []
        for source in self.__sources:
            thread = threading.Thread(target=source.process)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()


if __name__ == "__main__":
    service = Application()
    service.run()

