import os
from .File import File
from .LineIO import LineReader, LineWriter


class Files:
    def __init__(self, context):
        self.service = context.service
        self.context = context
        self.__files = []
        self.__files_by_key = {}

    def __get_file(self, file_type, name, key=None, **kw):

        def make_file(n):
            n = n if isinstance(n, str) else n()
            path = os.path.join(self.context.tmp_dir, n)
            return file_type(self.service, path, **kw)

        if key is None:
            f = make_file(name)
        else:
            try:
                return self.__files_by_key[key]
            except KeyError:
                f = make_file(name)
                self.__files_by_key[key] = f

        self.__files.append(f)

        return f

    def get_file(self, *args, **kw):
        return self.__get_file(File, *args, **kw)

    def get_line_reader(self, *args, **kw):
        return self.__get_file(LineReader, *args, **kw)

    def get_line_writer(self, *args, **kw):
        return self.__get_file(LineWriter, *args, **kw)

    def get_in_names(self, exclude=None):
        for _, _, files in os.walk(self.context.in_dir, True):
            for file in sorted(files):
                if file.startswith("."):
                    continue
                if exclude and exclude(file):
                    continue
                self.service.verify_running()
                yield file
            break

    def on_exit(self, is_error):
        for f in self.__files:
            if is_error:
                f.remove()
            else:
                path = os.path.join(self.context.out_dir,
                                    os.path.split(f.path)[1])
                self.service.print_(0, f"Output file {path}")
                f.move(self.context.out_dir)
