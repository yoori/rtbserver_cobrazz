import os
import argparse
import asyncio
import aiohttp
import psycopg2
import signal
import time


def get_sleep_subperiods(t):
    v = t / 0.1
    for i in range(int(v)):
        yield 0.1
    yield 0.1 * (v - int(v))


def make_keyword(name):
    r = str()
    for c in name:
        r += c if (('a' <= c <= 'z') or ('0' <= c <= '9')) else 'x'
    return r


SQL_QUERY = """DO $$DECLARE
  channel_id_val bigint;
  trigger_id_val bigint;
BEGIN
  SELECT adserver.get_taxonomy_channel(%s, %s, 'RU', 'ru') INTO channel_id_val;

  PERFORM adserver.insert_or_update_taxonomy_channel_parameters(channel_id_val, 'P', 10368000, 1);

  INSERT INTO triggers (trigger_type, normalized_trigger, qa_status, channel_type, country_code)
  SELECT 'K', %s, 'A', 'A', 'RU'
  WHERE NOT EXISTS (SELECT * FROM triggers WHERE trigger_type = 'K' AND normalized_trigger = %s AND channel_type = 'A' AND country_code = 'RU');

  SELECT trigger_id INTO trigger_id_val FROM triggers WHERE trigger_type = 'K' AND normalized_trigger = %s AND channel_type = 'A' AND country_code = 'RU';

  INSERT INTO channeltrigger(trigger_id, channel_id, channel_type, trigger_type, country_code, original_trigger, qa_status, negative)
  SELECT trigger_id_val, channel_id_val, 'A', 'P', 'RU', %s, 'A', false
  WHERE NOT EXISTS (SELECT * FROM channeltrigger WHERE trigger_id = trigger_id_val AND channel_id = channel_id_val AND channel_type = 'A' AND trigger_type = 'P' AND country_code = 'RU' AND original_trigger = %s);
END$$;""";


class Application:
    def __init__(self):
        self.running = True
        signal.signal(signal.SIGUSR1, self.__stop)

        parser = argparse.ArgumentParser()
        parser.add_argument("-upload-url", help="URL of server.")
        parser.add_argument("-period", type=float, help="Period between checking files.")
        parser.add_argument("-upload-wait-time", type=float, help="Time to wait for upload.")
        parser.add_argument("-dir", help="Folder with files.")
        parser.add_argument("-workspace-dir", help="Folder that stores the state ect.")
        parser.add_argument("-channel-prefix", help="Filename prefix.")
        parser.add_argument("-upload-threads", type=int, help="Maximum count of concurrent requests to update.")
        parser.add_argument("-pg-host", help="PostgreSQL hostname.")
        parser.add_argument("-pg-db", help="PostgreSQL DB name.")
        parser.add_argument("-pg-user", help="PostgreSQL user name.")
        parser.add_argument("-pg-pass", help="PostgreSQL password.")
        parser.add_argument("-account-id", type=int, help="Account ID.")
        parser.add_argument("-verbosity", type=int, default=1, help="Level of console information.")
        parser.add_argument("-print-line", type=int, default=0, help="Print line index despite verbosity.")
        parser.add_argument("--pid-file", required=False, help="File with process ID.")

        args = parser.parse_args()
        self.upload_url = args.upload_url
        self.period = args.period
        self.upload_wait_time = args.upload_wait_time
        self.dir = args.dir
        self.workspace_dir = args.workspace_dir
        self.markers_dir = os.path.join(self.workspace_dir, "markers")
        os.makedirs(self.markers_dir, exist_ok=True)
        self.channel_prefix = args.channel_prefix
        self.upload_threads = args.upload_threads
        self.verbosity = args.verbosity
        self.connection = psycopg2.connect(
            f"host='{args.pg_host}' dbname='{args.pg_db}' user='{args.pg_user}' password='{args.pg_pass}'")
        self.cursor = self.connection.cursor()
        self.account_id = args.account_id
        self.print_line = args.print_line
        self.line_index = 0
        self.pid_file = args.pid_file
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))

    def __stop(self, signum, frame):
        print("Stop signal")
        self.running = False

    def run(self):
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))
        try:
            asyncio.get_event_loop().run_until_complete(self.on_run())
        finally:
            if self.pid_file is not None:
                os.remove(self.pid_file)

    async def on_run(self):
        while True:
            await self.on_period()
            for t in get_sleep_subperiods(self.period):
                if not self.running:
                    return
                await asyncio.sleep(t)

    def __get_files_in_dir(self, dir_path):
        for root, dirs, files in os.walk(dir_path, True):
            return set(files)
        return set()

    async def on_period(self):
        files_in_dir = sorted(self.__get_files_in_dir(self.dir))
        files_in_markers = self.__get_files_in_dir(self.markers_dir)
        for file in files_in_dir:
            if not self.running:
                return
            reg_file = file + ".__reg__"
            if reg_file not in files_in_markers:
                if self.verbosity >= 1:
                    print("Reg file:", file)
                file_basename, file_ext = os.path.splitext(file)
                keyword = make_keyword(self.channel_prefix.lower() + file_basename.lower())
                self.cursor.execute(
                    SQL_QUERY,
                    (self.channel_prefix, self.account_id, keyword, keyword, keyword, keyword, keyword))
                reg_file_path = os.path.join(self.markers_dir, reg_file)
                with open(reg_file_path, "w"):
                    pass
        for file in files_in_dir:
            if not self.running:
                return
            file_path = os.path.join(self.dir, file)
            file_mtime = os.path.getmtime(file_path)
            upload_file = file + ".__upload__"
            upload_file_path = os.path.join(self.markers_dir, upload_file)
            if upload_file not in files_in_markers or file_mtime != os.path.getmtime(upload_file_path):
                reg_file = file + ".__reg__"
                reg_file_path = os.path.join(self.markers_dir, reg_file)
                reg_file_mtime = os.path.getmtime(reg_file_path)
                if reg_file_mtime + self.upload_wait_time <= time.time():
                    if self.verbosity >= 1:
                        print("Upload file:", file)
                    file_basename, file_ext = os.path.splitext(file)
                    keyword = make_keyword(self.channel_prefix.lower() + file_basename.lower())
                    is_stable = (file_ext == ".stable")
                    self.line_index = 0
                    with open(file_path, "r") as f:
                        await asyncio.gather(
                            *tuple(self.on_line(f, is_stable, keyword) for i in range(self.upload_threads)))
                    if not self.running:
                        return
                    with open(upload_file_path, "w") as f:
                        pass
                    os.utime(upload_file_path, (os.path.getatime(file_path), file_mtime))

    async def on_line(self, f, is_stable, keyword):

        async def get(uid):
            return await self.request(
                path="get",
                headers={"Host": "ad.new-programmatic.com", "Cookie": "uid=" + uid},
                params={"loc.name": "ru", "referer-kw": keyword})

        while True:
            if not self.running:
                break
            line = f.readline()
            if not line:
                break
            if line.endswith("\n"):
                line = line[:-1]
            self.line_index += 1
            print_line_index = self.verbosity >= 2 or (self.print_line and not (self.line_index % self.print_line))
            if print_line_index:
                print("line", self.line_index)
            if not is_stable:
                await get(line)
            else:
                session, resp = await self.request(
                    path="track.gif",
                    headers={},
                    params={"xid": "megafon-stableid/" + line, "u": "yUeKE9yKRKSu3bhliRyREA.."})
                uid = resp.cookies.get("uid")
                if uid is not None:
                    if print_line_index:
                        print("  has uid")
                    await get(uid.value)

    async def request(self, path, headers, params):
        url = f"{self.upload_url}/{path}"
        if self.verbosity >= 3:
            print("request ", "url=", url, "headers=", headers, "params=", params)
        async with aiohttp.ClientSession(headers=headers) as session:
            async with session.get(url=url, params=params, ssl=False) as resp:
                assert (resp.status == 204)
                return session, resp


def main():
    app = Application()
    app.run()


if __name__ == "__main__":
    main()

