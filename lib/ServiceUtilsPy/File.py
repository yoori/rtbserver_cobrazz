import os
import gzip
import shutil


class File:
    def __init__(self, service, path, mode=None, file=None, remove_on_exit=False, use_plugins=True):
        self.service = service
        self.path = path
        self.remove_on_exit = remove_on_exit
        if file is None:
            assert mode is not None
            if use_plugins and self.path.endswith(".gz"):
                file_type = gzip.open
            else:
                file_type = open
            self.file = file_type(path, mode)
            self.__need_close = True
        else:
            self.file = file
            self.__need_close = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.remove_on_exit:
            self.remove()
        else:
            self.close()

    def read(self, *args, **kw):
        self.service.verify_running()
        return self.file.read(*args, **kw)

    def write(self, *args, **kw):
        self.service.verify_running()
        return self.file.write(*args, **kw)

    def on_close(self):
        pass

    def close(self):
        if self.file is not None:
            if self.__need_close:
                self.file.close()
            self.file = None
            self.on_close()

    def remove(self):
        self.close()
        if os.path.isfile(self.path):
            os.remove(self.path)

    def move(self, path):
        self.service.print_(0, f"Output file {os.path.join(path, os.path.split(self.path)[1])}")
        self.close()
        shutil.move(self.path, path)

