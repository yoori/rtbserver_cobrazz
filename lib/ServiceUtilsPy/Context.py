import os
from datetime import datetime
from random import randint
import uuid
from .Files import Files
from .Markers import Markers


class Context:
    def __init__(self, service, in_dir=None, out_dir=None, markers_dir=None, tmp_dir=None):
        self.service = service

        def make_dir(dir, service_dir):
            dir = service_dir if dir is None else dir
            if dir is not None:
                os.makedirs(dir, exist_ok=True)
            return dir

        self.in_dir = make_dir(in_dir, service.in_dir)
        self.out_dir = make_dir(out_dir, service.out_dir)
        self.markers_dir = make_dir(markers_dir, service.markers_dir)
        self.tmp_dir = make_dir(tmp_dir, service.tmp_dir)

        now = datetime.now()
        self.fname_seed = f"{now.strftime('%Y%m%d')}.{now.strftime('%H%M%S')}.{now.strftime('%f')}.{randint(0, 99999999):08}"
        self.fname_stamp = ".stamp_" + now.strftime("%Y_%m_%d_%H_%M_%S_") + str(uuid.uuid4())

        self.files = Files(self)

        self.markers = Markers(self)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        is_error = exc_type is not None
        self.files.on_exit(is_error)
        self.markers.on_exit(is_error)

