import os
import argparse
import json
import time
import threading
import datetime
from random import randint


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
        self.print_lock = threading.Lock()
        self.verbosity = 2
        self.log_file = None

        def on_stop_signal():
            if not self.running:
                return
            self.print_(0, "Stop signal")
            self.on_stop_signal()

        try:
            from signal import SIGUSR1, signal
        except ImportError:
            pass
        else:
            signal(SIGUSR1, lambda signum, frame: on_stop_signal())

        try:
            from signal import SIGINT, signal
        except ImportError:
            pass
        else:
            signal(SIGINT, lambda signum, frame: on_stop_signal())

        self.args_parser = argparse.ArgumentParser()
        self.args_parser.add_argument("--period", type=float, help="Period between checking files.")
        self.args_parser.add_argument("--verbosity", type=int, help="Level of console information.")
        self.args_parser.add_argument("--pid-file", help="File with process ID.")
        self.args_parser.add_argument("--config", help="Path to JSON config.")
        self.args_parser.add_argument("--print-line", type=int, help="Print each line index.")
        self.args_parser.add_argument("--in-dir", help="Directory that stores input files.")
        self.args_parser.add_argument("--markers-dir", help="Directory that stores markers.")
        self.args_parser.add_argument("--tmp-dir", help="Directory that stores temp files.")
        self.args_parser.add_argument("--out-dir", help="Directory that stores output files.")
        self.args_parser.add_argument("--log-dir", help="Directory that stores log files.")
        self.args_parser.add_argument("--ulimit-files", help="The maximum number of open file descriptors.")

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
        self.log_dir = make_dir("log_dir")

        if self.log_dir is not None:
            now = datetime.datetime.now()
            log_fname = f"{now.strftime('%Y%m%d')}.{now.strftime('%H%M%S')}.{now.strftime('%f')}.{randint(0, 99999999):08}.txt"
            self.log_file = open(os.path.join(self.log_dir, log_fname), "w")

        self.pid_file = self.params.get("pid_file")
        if self.pid_file is not None:
            pid_dir = os.path.split(self.pid_file)[0]
            if pid_dir:
                os.makedirs(pid_dir, exist_ok=True)
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))

        ulimit_files = self.params.get("ulimit_files")
        if ulimit_files is not None:
            try:
                import resource
                v = resource.getrlimit(resource.RLIMIT_NOFILE)
                resource.setrlimit(resource.RLIMIT_NOFILE, (ulimit_files, v[1]))
            except ModuleNotFoundError:
                self.print_(0, "Can't set ulimit_files - module resource not found")

    def on_stop(self):
        if getattr(self, "pid_file", None) is not None:
            os.remove(self.pid_file)
        if getattr(self, "log_file", None) is not None:
            try:
                self.log_file.flush()
            except Exception as e:
                print(f"Can't flush log file - {e.__class__.__name__}:{str(e)}")
            self.log_file.close()

    def on_stop_signal(self):
        self.running = False

    def verify_running(self):
        if self.running:
            return
        raise StopService

    def print_(self, verbosity, text, flush=True):
        if verbosity <= self.verbosity:
            if isinstance(text, Exception):
                text = f"{text.__class__.__name__}:{str(text)}"
            self.print_lock.acquire()
            try:
                msg = f"{datetime.datetime.now()} - {threading.currentThread().name} - {text}"
                print(msg, flush=flush)
                if self.log_file is not None and not self.log_file.closed:
                    try:
                        self.log_file.write(msg + "\n")
                        if flush:
                            self.log_file.flush()
                    except Exception as e:
                        print(f"Can't write log file - {e.__class__.__name__}:{str(e)}", flush=flush)
            finally:
                self.print_lock.release()

    def on_timer(self):
        pass

    def run(self):
        try:
            self.print_(0, "Start")
            self.on_start()
            while True:
                self.print_(0, "Timer")
                self.on_timer()
                for t in self.__get_sleep_subperiods():
                    self.verify_running()
                    time.sleep(t)
        except StopService:
            pass
        finally:
            self.print_(0, "Stop")
            self.on_stop()

    def __get_sleep_subperiods(self):
        v = self.period / 0.1
        for i in range(int(v)):
            yield 0.1
        yield 0.1 * (v - int(v))

