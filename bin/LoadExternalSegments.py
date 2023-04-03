#!/usr/bin/python3

import os
import string
import logging
import urllib3.exceptions
from minio import Minio
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
            with self.create_context() as ctx:
                for name in sorted(obj.object_name for obj in objects):
                    self.service.verify_running()
                    if name == "meta.tsv":
                        continue
                    if not ctx.markers.add(name):
                        continue
                    if meta is None:
                        meta = self.__load_meta(client)
                    with MinioRequest(client, self.bucket, name) as mr:
                        with File(self.service, os.path.join(ctx.tmp_dir, name), "wb", remove_on_exit=True, use_plugins=False) as dl_writer:
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
                                        output_writer = ctx.files.get_line_writer(
                                            key=(segment_id, is_short),
                                            name=lambda: make_segment_filename(
                                                meta.get(segment_id, segment_id), is_short) + ctx.fname_stamp)
                                        output_writer.write_line(user_id)
        except urllib3.exceptions.MaxRetryError:
            logging.error("Minio error")

    def __load_meta(self, client):
        meta = {}
        with MinioRequest(client, self.bucket, "meta.tsv") as mr:
            for line in mr.data.decode("utf-8").split("\n"):
                if line:
                    meta_k, meta_v = line.split("\t")
                    meta[meta_k] = meta_v
        return meta


class FtpSource(Source):
    def __init__(self, service, params):
        super().__init__(service, params)
        mp = params["minio"]
        self.url = mp["url"]
        self.user = mp["user"]
        self.password = mp["password"]
        self.taxonomy_file = mp["taxonomy_file"]

    def process(self):
        raise NotImplementedError


class Application(Service):
    def __init__(self):
        super().__init__()
        self.sources = []

    def on_start(self):
        super().on_start()
        for source in self.config.get("sources", tuple()):
            self.sources.append(MinioSource(self, source) if "minio" in source else FtpSource(self, source))

    def on_run(self):
        for source in self.sources:
            source.process()


if __name__ == "__main__":
    service = Application()
    service.run()

