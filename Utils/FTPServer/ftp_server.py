import os
import argparse
import signal
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer


class Application:
    def __init__(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("-user", required=True, help="FTP username.")
        parser.add_argument("-password", required=True, help="FTP user password.")
        parser.add_argument("-dir", required=True, help="FTP directory.")
        parser.add_argument("-tmp_dir", required=True, help="Temporary directory.")
        parser.add_argument("-port", required=True, help="FTP port.")
        parser.add_argument("-max-cons", required=True, type=int, default=1024, help="Maximum connections.")
        parser.add_argument("-max-cons-per-ip", required=True, type=int, default=10,  help="Maximum connections per IP.")
        parser.add_argument("--pid-file", required=False, help="File with process ID")
        args = parser.parse_args()
        self.user = args.user
        self.password = args.password
        self.dir = args.dir
        os.makedirs(self.dir, exist_ok=True)
        self.tmp_dir = args.tmp_dir
        os.makedirs(self.tmp_dir, exist_ok=True)
        self.port = args.port
        self.pid_file = args.pid_file

        self.authorizer = DummyAuthorizer()
        self.authorizer.add_user(self.user, self.password, self.tmp_dir, perm="elradfmwMT")

        class Handler(FTPHandler):
            
            def on_file_received(self, file):
                fname = os.path.split(file)[1]
                os.rename(file, os.path.join(args.dir, fname))

            def on_incomplete_file_received(self, file):
                os.remove(file)

        self.handler = Handler
        self.handler.authorizer = self.authorizer

        self.server = FTPServer(("0.0.0.0", self.port), self.handler)
        self.server.max_cons = args.max_cons
        self.server.max_cons_per_ip = args.max_cons_per_ip

        self.running = True
        signal.signal(signal.SIGUSR1, self.__stop)

    def run(self):
        try:
            self.on_start()
        finally:
            self.on_stop()

    def on_start(self):
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))

        try:
            self.server.serve_forever()
        except:
            if self.running:
                raise

    def on_stop(self):
        if self.pid_file is not None:
            os.remove(self.pid_file)

    def __stop(self, signum, frame):
        print("Stop signal")
        self.running = False
        self.server.close_all()


def main():
    app = Application()
    app.run()


if __name__ == "__main__":
    main()

