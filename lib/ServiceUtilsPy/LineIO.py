from .File import File


class LineProgress:
    def __init__(self, service, name, verbosity):
        self.service = service
        self.name = name
        self.verbosity = verbosity
        self.index = 0

    def next_line(self):
        self.index += 1
        if self.service.print_line and not self.index % self.service.print_line:
            self.print_index()

    def print_index(self):
        self.service.print_(self.verbosity, f"Progress {self.name} - {self.index:,}")


class LineFile(File):
    def __init__(self, service, path=None, **kw):
        super().__init__(service, path=path, **kw)
        self.progress = LineProgress(self.service, path, 2)

    def on_close(self):
        self.progress.print_index()
        super().on_close()


class LineReader(LineFile):
    def __init__(self, service, path, **kw):
        super().__init__(service, path, mode="rt", **kw)

    def read_line(self):
        self.service.verify_running()
        line = self.file.readline()
        if line:
            line = line.strip("\n")
            self.progress.next_line()
        return line

    def read_lines(self):
        while True:
            line = self.read_line()
            if not line:
                return
            yield line


class LineWriter(LineFile):
    def __init__(self, service, path, **kw):
        super().__init__(service, path, mode="wt", **kw)
        self.first = True

    def write_line(self, line):
        self.service.verify_running()
        if self.first:
            self.first = False
        else:
            self.write("\n")
        self.write(line)
        self.progress.next_line()


