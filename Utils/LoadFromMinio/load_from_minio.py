import os
import argparse
import gzip
import shutil
import signal
from time import sleep
from minio import Minio


def get_sleep_subperiods(t):
    v = t / 0.1
    for i in range(int(v)):
        yield 0.1
    yield 0.1 * (v - int(v))


class Application:
    def __init__(self):
        self.plugins = {
            ".gz": self.on_extract_gz
        }

        self.running = True
        signal.signal(signal.SIGUSR1, self.__stop)

        parser = argparse.ArgumentParser()
        parser.add_argument("-url", help="URL of server.")
        parser.add_argument("-acc", help="Access key (login).")
        parser.add_argument("-secret", help="Secret key (password).")
        parser.add_argument("-bucket", help="Bucket name.")
        parser.add_argument("-out_dir", help="Directory to download files to.")
        parser.add_argument("-period", type=float, help="Period between attempts.")
        parser.add_argument("--tmp-dir", default="/tmp", help="Temp directory.")
        parser.add_argument("--pid-file", required=False, help="File with process ID.")
        args = parser.parse_args()
        self.url = args.url
        self.acc = args.acc
        self.secret = args.secret
        self.bucket = args.bucket
        self.out_dir = args.out_dir
        self.period = args.period
        self.tmp_dir = args.tmp_dir
        self.pid_file = args.pid_file

    def run(self):
        try:
            self.on_start()
        finally:
            self.on_stop()

    def on_start(self):
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))

        existing_files = set()
        if not os.path.isdir(self.out_dir):
            os.mkdir(self.out_dir)
        else:
            for root, dirs, files in os.walk(self.out_dir, True):
                existing_files.update(files)
                break

        client = Minio(self.url, access_key=self.acc, secret_key=self.secret)

        while self.running:
            objects = client.list_objects(self.bucket)
            for obj in objects:
                name = obj.object_name
                plugin = None
                for k, v in self.plugins.items():
                    if name.endswith(k):
                        name = name[:-len(k)]
                        plugin = v
                        break
                if plugin is None:
                    plugin = self.on_move_file
                if name not in existing_files:
                    print("Downloading " + obj.object_name)
                    response = client.get_object(self.bucket, obj.object_name)
                    try:
                        tmp_path = os.path.join(self.tmp_dir, obj.object_name)
                        file_path = os.path.join(self.out_dir, name)
                        try:
                            with open(tmp_path, "wb") as f:
                                for batch in response.stream():
                                    f.write(batch)
                            plugin(tmp_path, file_path)
                        except:
                            self.__safe_remove(file_path)
                            raise
                        finally:
                            self.__safe_remove(tmp_path)
                    finally:
                        response.close()
                        response.release_conn()
                    existing_files.add(name)
            for t in get_sleep_subperiods(self.period):
                if not self.running:
                    return
                sleep(t)

    def on_stop(self):
        if self.pid_file is not None:
            os.remove(self.pid_file)

    def on_move_file(self, path_in, path_out):
        shutil.move(path_in, path_out)

    def on_extract_gz(self, path_in, path_out):
        path_tmp = path_in + ".tml" 
        try:
            with gzip.open(path_in, "rb") as f_in:
                with open(path_tmp, "wb") as f_out:
                    shutil.copyfileobj(f_in, f_out)
            self.on_move_file(path_tmp, path_out)
        finally:
            self.__safe_remove(path_tmp)

    def __stop(self, signum, frame):
        print("Stop signal received");
        self.running = False

    def __safe_remove(self, path):
        if os.path.isfile(path):
            os.remove(path)


def main():
    app = Application()
    app.run()


if __name__ == "__main__":
    main()

