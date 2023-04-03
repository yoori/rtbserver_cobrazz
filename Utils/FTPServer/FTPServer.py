#!/usr/bin/python3

import os
import shutil
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer
from ServiceUtilsPy.Service import Service, StopService


class Application(Service):
    def __init__(self):
        super().__init__()

        self.args_parser.add_argument("--user", help="FTP username.")
        self.args_parser.add_argument("--password", help="FTP user password.")
        self.args_parser.add_argument("--port", help="FTP port.")
        self.args_parser.add_argument("--max-cons", type=int, help="Maximum connections.")
        self.args_parser.add_argument("--max-cons-per-ip", type=int, help="Maximum connections per IP.")

    def on_start(self):
        super().on_start()

        self.user = self.params["user"]
        self.password = self.params["password"]
        self.port = self.params["port"]

        self.authorizer = DummyAuthorizer()
        self.authorizer.add_user(self.user, self.password, self.tmp_dir, perm="elradfmwMT")

        class Handler(FTPHandler):
            OUT_DIR = self.out_dir

            def on_file_received(self, file):
                shutil.move(file, self.OUT_DIR)

            def on_incomplete_file_received(self, file):
                os.remove(file)

        self.handler = Handler
        self.handler.authorizer = self.authorizer

        self.server = FTPServer(("0.0.0.0", self.port), self.handler)
        self.server.max_cons = self.params.get("max_cons", 1024)
        self.server.max_cons_per_ip = self.params.get("max_cons_per_ip", 10)

    def on_run(self):
        try:
            self.server.serve_forever()
        except:
            if self.running:
                raise
        else:
            raise StopService

    def on_stop_signal(self):
        super().on_stop_signal()
        self.server.close_all()


if __name__ == "__main__":
    service = Application()
    service.run()

