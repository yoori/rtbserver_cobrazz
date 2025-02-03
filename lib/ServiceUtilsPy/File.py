import os
import gzip
import shutil


PLUGINS = {}


def add_plugin(plugin_ext, plugin_open):
    if plugin_ext in PLUGINS:
        raise RuntimeError(f"File plugin {plugin_ext} already added")
    PLUGINS[plugin_ext] = plugin_open


add_plugin(".gz", gzip.open)


class File:
    def __init__(
            self, service, path, mode=None, file=None, remove_on_exit=False,
            use_plugins=True):
        self.service = service
        self.path = path
        self.remove_on_exit = remove_on_exit
        if file is None:
            if mode is None:
                raise RuntimeError("File mode is None")
            file_open = open
            if use_plugins:
                for plugin_ext, plugin_open in PLUGINS.items():
                    if self.path.endswith(plugin_ext):
                        file_open = plugin_open
                        break
            self.file = file_open(path, mode)
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
        self.close()
        shutil.move(self.path, path)
