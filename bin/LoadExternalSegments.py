#! /usr/bin/env python3

import os
import string
import urllib3.exceptions
import requests
import threading
from minio import Minio
from lxml import etree
from ServiceUtilsPy.File import File
from ServiceUtilsPy.LineIO import LineReader
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context
from ServiceUtilsPy.Minio import MinioRequest


try:
    from minio.error import MinioException
except ImportError:
    from minio.error import MinioError as MinioException


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


class Source:
    def __init__(self, service, params):
        self.service = service
        self.markers_dir = params["markers_dir"]
        self.tmp_dir = params["tmp_dir"]
        self.out_dir = params["out_dir"]

    def create_context(self, *args, **kw):
        return Context(*args, service=self.service, markers_dir=self.markers_dir, tmp_dir=self.tmp_dir, out_dir=self.out_dir, **kw)


class MinioSource(Source):
    def __init__(self, service, params):
        super().__init__(service, params)
        mp = params["minio"]
        self.url = mp["url"]
        self.acc = mp["acc"]
        self.secret = mp["secret"]
        self.bucket = mp["bucket"]

    def process(self):
        meta = None
        client = Minio(self.url, access_key=self.acc, secret_key=self.secret)
        objects = client.list_objects(self.bucket)
        try:
            for name in sorted(obj.object_name for obj in objects):
                self.service.verify_running()
                if name == "meta.tsv":
                    continue
                self.service.print_(1, f"{name}")
                with self.create_context() as ctx:
                    if not ctx.markers.add(name):
                        continue
                    meta = self.__load_meta(client)
                    with MinioRequest(client, self.bucket, name) as mr:
                        with File(self.service, os.path.join(ctx.tmp_dir, name), "wb",
                                  remove_on_exit=True, use_plugins=False) as dl_writer:
                            self.service.print_(1, f"Downloading {name}")
                            for batch in mr.stream():
                                dl_writer.write(batch)
                            dl_writer.close()

                            self.service.print_(1, f"Processing {name}")
                            with LineReader(self.service, dl_writer.path) as dl_reader:
                                for line in dl_reader.read_lines():
                                    user_id, segment_ids = line.split("\t")
                                    is_short = len(user_id) < 32
                                    for segment_id in segment_ids.split(","):
                                        segment_id = meta.get(segment_id, segment_id)
                                        output_writer = ctx.files.get_line_writer(
                                            key=(segment_id, is_short),
                                            name=lambda: make_segment_filename(segment_id, is_short) + ctx.fname_stamp)
                                        output_writer.progress.verbosity = 3
                                        output_writer.write_line(user_id)
        except MinioException as e:
            self.service.print_(0, e)
        except urllib3.exceptions.HTTPError as e:
            self.service.print_(0, e)
        except EOFError as e:
            self.service.print_(0, e)

    def __load_meta(self, client):
        meta = {}
        with MinioRequest(client, self.bucket, "meta.tsv") as mr:
            for line in mr.data.decode("utf-8").split("\n"):
                if line:
                    meta_k, meta_v = line.split("\t")
                    meta[meta_k] = meta_v
        return meta


class HTTPSource(Source):
    def __init__(self, service, params):
        super().__init__(service, params)
        mp = params["http"]
        self.__url = f'https://{mp["user"]}:{mp["password"]}@{mp["url"]}'
        self.__taxonomy = {}
        with LineReader(self.service, mp["taxonomy_file"]) as f:
            for line in f.read_lines():
                if line:
                    segment_id, segment = line.split("\t")
                    if segment:
                        self.__taxonomy[segment_id] = segment

    def process(self):
        try:
            with requests.get(self.__url) as files_response:
                if files_response.status_code != 200:
                    raise requests.exceptions.RequestException
                tree = etree.HTML(files_response.text)
                for name in tree.xpath("/html/body/pre/a/text()"):
                    if name == "../":
                        continue
                    self.service.print_(1, f"{name}")
                    with self.create_context() as ctx:
                        if not ctx.markers.add(name):
                            continue
                        with requests.get(f"{self.__url}/{name}", stream=True) as file_response:
                            if file_response.status_code != 200:
                                raise requests.exceptions.RequestException
                            with File(self.service, os.path.join(ctx.tmp_dir, name), "wb",
                                      remove_on_exit=True, use_plugins=False) as dl_writer:
                                self.service.print_(1, f"Downloading {name}")
                                for chunk in file_response.iter_content(chunk_size=65536):
                                    dl_writer.write(chunk)
                                dl_writer.close()

                                self.service.print_(1, f"Processing {name}")
                                with LineReader(self.service, dl_writer.path) as dl_reader:
                                    for line in dl_reader.read_lines():
                                        user_id, segment_ids = line.split("\t")
                                        is_short = len(user_id) < 32
                                        for segment_id in segment_ids.split(","):
                                            segment_id = self.__taxonomy.get(segment_id, segment_id)
                                            output_writer = ctx.files.get_line_writer(
                                                key=(segment_id, is_short),
                                                name=lambda: make_segment_filename(segment_id, is_short) + ctx.fname_stamp)
                                            output_writer.progress.verbosity = 3
                                            output_writer.write_line(user_id)
        except requests.exceptions.RequestException as e:
            self.service.print_(0, e)
        except EOFError as e:
            self.service.print_(0, e)


class Application(Service):
    def __init__(self):
        super().__init__()
        self.sources = []

    def on_start(self):
        super().on_start()
        for source in self.config.get("sources", tuple()):
            minio = source.get("minio")
            if minio is not None:
                self.sources.append(MinioSource(self, source))
                continue
            http = source.get("http")
            if http is not None:
                self.sources.append(HTTPSource(self, source))
                continue
            raise RuntimeError("unknown source type")

    def on_run(self):
        threads = []
        for source in self.sources:
            thread = threading.Thread(target=source.process)
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()


if __name__ == "__main__":
    service = Application()
    service.run()

