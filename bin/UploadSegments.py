#!/usr/bin/python3

import os
import argparse
import json
import asyncio
import aiohttp
import psycopg2
import signal
import subprocess
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


SQL_REG_USER = """DO $$DECLARE
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
END$$;"""


SQL_REG_URL = "SELECT adserver.get_taxonomy_channel(%s, 115, 'RU', 'ru');"


SQL_UPLOAD_URL = """DO $$DECLARE
  trigger_id_val bigint;
BEGIN
  SELECT trigger_id INTO trigger_id_val FROM triggers WHERE trigger_type = 'U' AND
    normalized_trigger = %s AND channel_type = 'A' AND country_code = 'RU';

  INSERT INTO channeltrigger(trigger_id, channel_id, channel_type, trigger_type, country_code, original_trigger, qa_status, negative)
  SELECT trigger_id_val, %s, 'A', 'U', 'RU', %s, 'A', false
  WHERE NOT EXISTS (SELECT * FROM channeltrigger WHERE trigger_id = trigger_id_val AND channel_id = %s AND channel_type = 'A' AND
    trigger_type = 'U' AND country_code = 'RU' AND original_trigger = %s);
END$$;"""


class Application:
    def __init__(self):
        self.running = True
        signal.signal(signal.SIGUSR1, self.__stop)

        parser = argparse.ArgumentParser()
        parser.add_argument("--upload-url", help="URL of server.")
        parser.add_argument("--period", type=float, help="Period between checking files.")
        parser.add_argument("--upload-wait-time", type=float, help="Time to wait for upload.")
        parser.add_argument("--dir", help="Folder with files.")
        parser.add_argument("--workspace-dir", help="Folder that stores the state ect.")
        parser.add_argument("--channel-prefix", help="Filename prefix.")
        parser.add_argument("--upload-threads", type=int, help="Maximum count of concurrent requests to update.")
        parser.add_argument("--pg-host", help="PostgreSQL hostname.")
        parser.add_argument("--pg-db", help="PostgreSQL DB name.")
        parser.add_argument("--pg-user", help="PostgreSQL user name.")
        parser.add_argument("--pg-pass", help="PostgreSQL password.")
        parser.add_argument("--account-id", type=int, help="Account ID.")
        parser.add_argument("--verbosity", type=int, help="Level of console information.")
        parser.add_argument("--print-line", type=int, help="Print line index despite verbosity.")
        parser.add_argument("--pid-file", help="File with process ID.")
        parser.add_argument("--private-key-file", help="Private .der key for signing uids.")
        parser.add_argument("--url-segments-dir", help="Private .der key for signing uids.")
        parser.add_argument("--config", default=None, help="Path to JSON config.")

        args = parser.parse_args()

        if args.config is None:
            config = {}
        else:
            with open(args.config, "r") as f:
                config = json.load(f)

        def get_param(name, *default):
            v = getattr(args, name)
            if v is not None:
                return v
            v = config.get(name)
            if v is not None:
                return v
            assert(0 <= len(default) <= 1)
            if len(default) == 0:
                raise RuntimeError(f"param '{name}' not found")
            return default[0]

        self.upload_url = get_param("upload_url")
        self.period = get_param("period")
        self.upload_wait_time = get_param("upload_wait_time")
        self.dir = get_param("dir")
        self.workspace_dir = get_param("workspace_dir")
        self.markers_dir = os.path.join(self.workspace_dir, "markers")
        os.makedirs(self.markers_dir, exist_ok=True)
        self.channel_prefix = get_param("channel_prefix", None)
        self.upload_threads = get_param("upload_threads")
        self.verbosity = get_param("verbosity", 1)
        ph_host = get_param("pg_host")
        pg_db = get_param("pg_db")
        pg_user = get_param("pg_user")
        pg_pass = get_param("pg_pass")
        self.connection = psycopg2.connect(
            f"host='{ph_host}' dbname='{pg_db}' user='{pg_user}' password='{pg_pass}'")
        self.cursor = self.connection.cursor()
        self.account_id = get_param("account_id")
        self.print_line = get_param("print_line", 0)
        self.line_index = 0
        self.pid_file = get_param("pid_file")
        self.private_key_file = get_param("private_key_file")
        if self.pid_file is not None:
            with open(self.pid_file, "w") as f:
                f.write(str(os.getpid()))
        self.url_segments_dir = get_param("url_segments_dir")
        if self.url_segments_dir is not None:
            self.url_markers_dir = os.path.join(self.workspace_dir, "url_markers")
            os.makedirs(self.url_markers_dir, exist_ok=True)

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
            return files
        return tuple()

    async def on_period(self):
        await self.on_uids()
        await self.on_urls()

    async def on_uids(self):
        if self.channel_prefix is not None:
            await self.on_uids_dir(self.dir, self.markers_dir, self.channel_prefix)
        for root, dirs, files in os.walk(self.dir, True):
            for dir in dirs:
                if not self.running:
                    break
                markers_dir = os.path.join(self.markers_dir, dir)
                os.makedirs(markers_dir, exist_ok=True)
                await self.on_uids_dir(os.path.join(self.dir, dir), markers_dir, dir)
            break

    async def on_uids_dir(self, dir, markers_dir, channel_prefix):
        files_in_dir = sorted(self.__get_files_in_dir(dir))
        files_in_markers = set(self.__get_files_in_dir(markers_dir))

        for file in files_in_dir:
            if not self.running:
                return

            file_path = os.path.join(dir, file)
            file_mtime = os.path.getmtime(file_path)

            signed_uids = False
            basename, ext = os.path.splitext(file)
            if ext.startswith(".stamp"):
                basename, ext = os.path.splitext(basename)
            if ext.startswith(".signed_uids"):
                basename, ext = os.path.splitext(basename)
                signed_uids = True
            basename, ext = os.path.splitext(basename)

            reg_file = basename + ".__reg__"
            reg_file_path = os.path.join(markers_dir, reg_file)

            upload_file = file + ".__upload__"
            upload_file_path = os.path.join(markers_dir, upload_file)

            keyword = make_keyword(channel_prefix.lower() + basename.lower())

            if reg_file not in files_in_markers:
                files_in_markers.add(reg_file)
                if self.verbosity >= 1:
                    print("Registering file:", basename)
                self.cursor.execute(
                    SQL_REG_USER,
                    (self.channel_prefix + basename.upper(), self.account_id, keyword, keyword, keyword, keyword, keyword))
                self.cursor.execute("COMMIT;")
                with open(reg_file_path, "w"):
                    pass

            if upload_file not in files_in_markers or file_mtime != os.path.getmtime(upload_file_path):
                if os.path.getmtime(reg_file_path) + self.upload_wait_time <= time.time():
                    if self.verbosity >= 1:
                        print("Uploading file:", file)
                    is_stable = (ext == ".stable")
                    self.line_index = 0

                    async def run_lines(f):
                        await asyncio.gather(
                            *tuple(self.on_line(f, is_stable, keyword) for i in range(self.upload_threads)))

                    if not signed_uids and ext in ("", ".txt", ".uids"):
                        with subprocess.Popen(
                            ['sh', '-c', f'cat "{file_path}" | sed -r "s/([^.])$/\\1../" | UserIdUtil sign-uid --private-key-file="{self.private_key_file}"'],
                                stdout=subprocess.PIPE) as shp:
                            await run_lines(shp.stdout)
                    else:
                        with open(file_path, "rb") as f:
                            await run_lines(f)
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
            line = f.readline().decode("utf-8")
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

    async def on_urls(self):
        if self.url_segments_dir is None:
            return

        files_in_dir = sorted(self.__get_files_in_dir(self.url_segments_dir))
        files_in_markers = set(self.__get_files_in_dir(self.url_markers_dir))

        for file in files_in_dir:
            if not self.running:
                return

            file_path = os.path.join(self.url_segments_dir, file)
            file_mtime = os.path.getmtime(file_path)

            reg_file = file + ".__reg__"
            reg_file_path = os.path.join(self.url_markers_dir, reg_file)

            if reg_file not in files_in_markers or file_mtime != os.path.getmtime(reg_file_path):
                if self.verbosity >= 1:
                    print("Registering URL file:", file)
                self.cursor.execute(
                    SQL_REG_URL,
                    (self.channel_prefix + file.upper(),))
                channel_id = self.cursor.fetchone()[0]
                self.cursor.execute("COMMIT;")
                if self.verbosity >= 1:
                    print("Uploading URL file:", file)
                with open(file_path, "rt") as f:
                    while self.running:
                        line = f.readline()
                        if not line:
                            break
                        if line.endswith("\n"):
                            line = line[:-1]
                        if not line:
                            continue
                        self.cursor.execute(
                            SQL_UPLOAD_URL,
                            (line, channel_id, line, channel_id, line))
                        self.cursor.execute("COMMIT;")
                if not self.running:
                    return
                with open(reg_file_path, "w"):
                    pass
                os.utime(reg_file_path, (os.path.getatime(file_path), file_mtime))


def main():
    app = Application()
    app.run()


if __name__ == "__main__":
    main()

