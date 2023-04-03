import os
import argparse
import json
import asyncio
import time


class Params:
    def __init__(self, service):
        self.service = service

    def get(self, name, default=None):
        try:
            return self[name]
        except KeyError:
            return default

    def __getitem__(self, name):
        v = getattr(self.service.args, name)
        if v is not None:
            return v
        return self.service.config[name]


class StopService(Exception):
    pass


class Service:
    def __init__(self):
        self.running = True

        try:
            from signal import SIGUSR1, signal
        except ImportError:
            pass
        else:
            signal(SIGUSR1, lambda signum, frame: self.on_stop_signal())

        self.args_parser = argparse.ArgumentParser()
        self.args_parser.add_argument("--period", type=float, help="Period between checking files.")
        self.args_parser.add_argument("--verbosity", type=int, help="Level of console information.")
        self.args_parser.add_argument("--pid-file", help="File with process ID.")
        self.args_parser.add_argument("--config", help="Path to JSON config.")
        self.args_parser.add_argument("--print-line", type=int, help="Print line index despite verbosity.")
        self.args_parser.add_argument("--in-dir", help="Directory that stores input files.")
        self.args_parser.add_argument("--markers-dir", help="Directory that stores markers.")
        self.args_parser.add_argument("--tmp-dir", help="Directory that stores temp files.")
        self.args_parser.add_argument("--out-dir", help="Directory that stores output files.")

    def on_start(self):
        self.args = self.args_parser.parse_args()

        if self.args.config is None:
            self.config = {}
        else:
            with open(self.args.config, "r") as f:
                self.config = json.load(f)

        self.params = Params(self)
        self.period = self.params.get("period", 0)
        self.verbosity = self.params.get("verbosity", 1)
        self.print_line = self.params.get("print_line", 10000)

        def make_dir(name):
            d = self.params.get(name)
            if d is not None:
                os.makedirs(d, exist_ok=True)
            return d

        self.in_dir = make_dir("in_dir")
        self.out_dir = make_dir("out_dir")
        self.markers_dir = make_dir("markers_dir")
        self.tmp_dir = make_dir("tmp_dir")

        self.pid_file = self.params.get("pid_file")
        if self.pid_file is not None:
            pid_dir = os.path.split(self.pid_file)[0]
            if pid_dir:
                os.makedirs(pid_dir, exist_ok=True)
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))

    def on_stop(self):
        if getattr(self, "pid_file", None) is not None:
            os.remove(self.pid_file)

    def on_stop_signal(self):
        print("Stop signal")
        self.running = False

    def verify_running(self):
        if self.running:
            return
        raise StopService

    def print_(self, verbosity, *args, **kw):
        if verbosity <= self.verbosity:
            print(*args, **kw)

    def on_run(self):
        pass

    def run(self):
        try:
            self.on_start()
            while True:
                self.on_run()
                for t in self.__get_sleep_subperiods():
                    self.verify_running()
                    time.sleep(t)
        except StopService:
            pass
        finally:
            self.on_stop()

    async def on_run_async(self):
        pass

    def run_async(self):
        self.on_start()

        async def f():
            while True:
                await self.on_run_async()
                for t in self.__get_sleep_subperiods():
                    self.verify_running()
                    await asyncio.sleep(t)

        try:
            asyncio.get_event_loop().run_until_complete(f())
        except StopService:
            pass
        finally:
            self.on_stop()

    def __get_sleep_subperiods(self):
        v = self.period / 0.1
        for i in range(int(v)):
            yield 0.1
        yield 0.1 * (v - int(v))

