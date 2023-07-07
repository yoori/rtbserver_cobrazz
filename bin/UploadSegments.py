#! /usr/bin/env python3

import os
import io
import aiohttp
import aiohttp.client_exceptions
import psycopg2
import psutil
import subprocess
import asyncio
import threading
import random
import shutil
from ServiceUtilsPy.Service import Service, StopService
from ServiceUtilsPy.Context import Context
from ServiceUtilsPy.LineIO import LineReader


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


class Upload:
    def __init__(self, params):
        self.account_id = params["account_id"]
        self.channel_prefix = params["channel_prefix"]
        self.in_dir = params["in_dir"]
        workspace_dir = params["workspace_dir"]
        self.markers_dir = os.path.join(workspace_dir, "markers")
        self.url_segments_dir = params.get("url_segments_dir")
        if self.url_segments_dir is not None:
            self.url_markers_dir = os.path.join(workspace_dir, "url_markers")


class FileInfo:
    def __init__(self, in_name):
        signed_uids = False
        basename, ext = os.path.splitext(in_name)
        if ext.startswith(".stamp"):
            basename, ext = os.path.splitext(basename)
        if ext.startswith(".signed_uids"):
            basename, ext = os.path.splitext(basename)
            signed_uids = True
        if ext in (".stable", ".uids", ".txt"):
            self.is_stable = (ext == ".stable")
            basename, ext = os.path.splitext(basename)
        else:
            self.is_stable = False
        self.reg_name = basename + ext
        self.need_sign_uids = not signed_uids and ext in ("", ".txt", ".uids")

    def __repr__(self):
        return f"FileInfo(reg_name={self.reg_name}, need_sign_uids={self.need_sign_uids}, is_stable={self.is_stable})"


class Application(Service):
    def __init__(self):
        super().__init__()

        self.args_parser.add_argument("--upload-url", help="URL of server.")
        self.args_parser.add_argument("--upload-wait-time", type=float, help="Time to wait for upload.")
        self.args_parser.add_argument("--account-id", type=int, help="Account ID.")
        self.args_parser.add_argument("--channel-prefix", help="Filename prefix.")
        self.args_parser.add_argument("--workspace-dir", help="Folder that stores the state ect.")
        self.args_parser.add_argument("--url-segments-dir", help="Private .der key for signing uids.")
        self.args_parser.add_argument("--upload-threads", type=int, help="Maximum count of concurrent requests to update.")
        self.args_parser.add_argument("--pg-host", help="PostgreSQL hostname.")
        self.args_parser.add_argument("--pg-db", help="PostgreSQL DB name.")
        self.args_parser.add_argument("--pg-user", help="PostgreSQL user name.")
        self.args_parser.add_argument("--pg-pass", help="PostgreSQL password.")
        self.args_parser.add_argument("--private-key-file", help="Private .der key for signing uids.")

        self.__subprocesses = set()
        self.__subprocesses_lock = threading.Lock()

    def on_start(self):
        super().on_start()

        self.upload_url = self.params["upload_url"]
        if isinstance(self.upload_url, str):
            self.upload_url = [self.upload_url]
        self.upload_wait_time = self.params["upload_wait_time"]

        self.uploads = []
        if self.params.get("account_id") is not None:
            self.uploads.append(Upload(self.params))
        for upload in self.config.get("uploads", tuple()):
            self.uploads.append(Upload(upload))

        self.upload_threads = self.params["upload_threads"]

        self.private_key_file = self.params["private_key_file"]

        if shutil.which("UserIdUtil") is None:
            raise RuntimeError("UserIdUtil not found")

    def on_stop_signal(self):
        with self.__subprocesses_lock:
            for pid in self.__subprocesses:
                parent = psutil.Process(pid=pid)
                for child in parent.children(recursive=True):
                    try:
                        child.kill()
                    except:
                        pass
                try:
                    parent.kill()
                except:
                    pass
        super().on_stop_signal()

    def on_run(self):
        threads = []
        for upload in self.uploads:
            thread = threading.Thread(target=self.on_thread, args=(upload,))
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

    def on_thread(self, upload):
        ph_host = self.params["pg_host"]
        pg_db = self.params["pg_db"]
        pg_user = self.params["pg_user"]
        pg_pass = self.params["pg_pass"]
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        connection = None
        try:
            connection = psycopg2.connect(
                f"host='{ph_host}' dbname='{pg_db}' user='{pg_user}' password='{pg_pass}'")
            cursor = connection.cursor()
            loop.run_until_complete(self.on_uids(upload, cursor))
            loop.run_until_complete(self.on_urls(upload, cursor))
        except psycopg2.Error as e:
            self.print_(0, e)
        except StopService:
            pass
        finally:
            if connection is not None:
                connection.close()

    async def on_uids(self, upload, cursor):
        if upload.channel_prefix is not None:
            await self.on_uids_dir(cursor, upload.in_dir, upload.markers_dir, upload.channel_prefix, upload.account_id)
        for root, dirs, files in os.walk(upload.in_dir, True):
            for subdir in dirs:
                await self.on_uids_dir(
                    cursor,
                    os.path.join(upload.in_dir, subdir),
                    os.path.join(upload.markers_dir, subdir),
                    subdir,
                    upload.account_id)
            break

    async def on_uids_dir(self, cursor, in_dir, markers_dir, channel_prefix, account_id):
        self.print_(1, f"In dir {in_dir}")
        self.print_(1, f"Markers dir {markers_dir}")
        try:
            with Context(self, in_dir=in_dir) as in_dir_ctx:
                with Context(self, markers_dir=markers_dir) as markers_ctx:
                    unprocessed_count = len(tuple(in_dir_ctx.files.get_in_files()))
                self.print_(0, f"Unprocessed files count: {unprocessed_count}")

                while True:
                    self.verify_running()
                    with Context(self, markers_dir=markers_dir) as markers_ctx:
                        in_files = tuple(in_dir_ctx.files.get_in_files())
                        if not in_files:
                            break
                        file_info = FileInfo(max(
                            in_files,
                            key=lambda in_file_: os.path.getmtime(os.path.join(in_dir, in_file_))))
                        reg_marker_name = file_info.reg_name + ".__reg__"
                        keyword = make_keyword(channel_prefix.lower() + file_info.reg_name.lower())
                        if markers_ctx.markers.add(reg_marker_name):
                            channel_id = channel_prefix + file_info.reg_name.upper()
                            self.print_(1, f"Registering file: {reg_marker_name} ({in_name}) channel_id {channel_id} account_id {account_id} keyword {keyword}")
                            cursor.execute(
                                SQL_REG_USER,
                                (channel_id, account_id, keyword, keyword, keyword, keyword, keyword))
                            cursor.execute("COMMIT;")

                        if not markers_ctx.markers.check_mtime_interval(reg_marker_name, self.upload_wait_time):
                            continue

                        in_names = []
                        for in_file in in_files:
                            if repr(file_info) != repr(FileInfo(in_file)):
                                continue
                            in_names.append(in_file)
                        in_names.sort(
                            key=lambda _: os.path.getmtime(os.path.join(in_dir, _)),
                            reverse=True)
                        in_names = in_names[:12]

                        for in_name_index, in_name in enumerate(in_names):
                            if in_name_index == 0:
                                self.print_(1, f"Processing file group: {file_info}")
                            self.print_(1, f" {in_name_index + 1}) File {in_name}")

                        if len(in_names) > 1:
                            file_groups = {
                                0: dict(
                                    (in_name, open(os.path.join(in_dir, in_name), "rb"))
                                    for in_name in in_names)
                            }
                            in_names.clear()

                            while file_groups:
                                new_file_groups = {}
                                for group_id, file_group in file_groups.items():
                                    for in_name, f in file_group.items():
                                        data = b"" if len(file_group) == 1 else f.read(8000)
                                        try:
                                            new_file_group = new_file_groups[(group_id, data)]
                                        except KeyError:
                                            new_file_groups[(group_id, data)] = {in_name: f}
                                            if not data:
                                                f.close()
                                                in_names.append(in_name)
                                        else:
                                            new_file_group[in_name] = f
                                            if not data:
                                                f.close()
                                                self.print_(1, f"Duplicate file: {in_name}")
                                                os.remove(os.path.join(in_dir, in_name))
                                file_groups.clear()
                                new_group_ids = {}
                                for (group_id, data), new_file_group in new_file_groups.items():
                                    if data:
                                        try:
                                            new_group_id = new_group_ids[(group_id, data)]
                                        except KeyError:
                                            new_group_id = len(new_group_ids)
                                            new_group_ids[(group_id, data)] = new_group_id
                                        file_groups[new_group_id] = new_file_group

                        self.print_(0, f"Unique file count: {len(in_names)}")

                        async def run_lines(f):
                            async with aiohttp.ClientSession() as session:
                                await asyncio.gather(
                                    *tuple(self.on_line(f, file_info.is_stable, keyword, session)
                                        for _ in range(self.upload_threads)))

                        in_paths_str = " ".join(os.path.join(in_dir, in_name) for in_name in in_names)

                        if file_info.need_sign_uids:
                            cmd = ['sh', '-c', f'cat {in_paths_str} | sed -r "s/([^.])$/\\1../" | sort -u --parallel=4 | UserIdUtil sign-uid --private-key-file="{self.private_key_file}"']
                        else:
                            cmd = ['sh', '-c', f'cat {in_paths_str} | sort -u --parallel=4']

                        with subprocess.Popen(cmd, stdout=subprocess.PIPE) as shp:
                            with self.__subprocesses_lock:
                                self.__subprocesses.add(shp.pid)
                            try:
                                file = io.TextIOWrapper(io.BufferedReader(shp.stdout, buffer_size=65536), encoding="utf-8")
                                self.print_(0, f"Processing...")
                                with LineReader(self, path=file_info.reg_name, file=file) as f:
                                    await run_lines(f)
                            finally:
                                with self.__subprocesses_lock:
                                    self.__subprocesses.remove(shp.pid)

                        self.verify_running()
                        if shp.returncode >= 0:
                            for in_name in in_names:
                                self.print_(1, f"Done file: {in_name}")
                                os.remove(os.path.join(in_dir, in_name))

        except aiohttp.client_exceptions.ClientError as e:
            self.print_(0, e)
        except EOFError as e:
            self.print_(0, e)

    async def on_line(self, f, is_stable, keyword, session):
        async def get(uid):
            await self.request(
                session,
                path="get",
                headers={"Host": "ad.new-programmatic.com", "Cookie": "uid=" + uid},
                params={"loc.name": "ru", "referer-kw": keyword})

        try:
            for line in f.read_lines():
                if not is_stable:
                    await get(line)
                else:

                    async def visitor(session, resp):
                        uid = resp.cookies.get("uid")
                        if uid is not None:
                            await get(uid.value)

                    await self.request(
                        session,
                        path="track.gif",
                        headers={},
                        params={"xid": "megafon-stableid/" + line, "u": "yUeKE9yKRKSu3bhliRyREA.."},
                        visitor=visitor)
        except StopService:
            pass

    async def request(self, session, path, headers, params, visitor=None):
        url = f"{random.choice(self.upload_url)}/{path}"
        self.print_(3, f"Request url={url} headers={headers} params={params}")
        try:
            async with session.get(url=url, params=params, headers=headers, ssl=False) as resp:
                if resp.status != 204:
                    raise aiohttp.client_exceptions.ClientResponseError(resp.request_info, resp.history)
                if visitor is not None:
                    await visitor(session, resp)
        except aiohttp.client_exceptions.ClientError as e:
            self.print_(0, e)

    async def on_urls(self, upload, cursor):
        if upload.url_segments_dir is None:
            return

        try:
            with Context(self, in_dir=upload.url_segments_dir, markers_dir=upload.url_markers_dir) as ctx:
                for in_name in ctx.files.get_in_files():
                    in_path = os.path.join(ctx.in_dir, in_name)
                    if ctx.markers.add(in_name, os.path.getmtime(in_path)):
                        self.print_(1, f"Registering URL file: {in_name}")
                        cursor.execute(
                            SQL_REG_URL,
                            (upload.channel_prefix + in_name.upper(),))
                        channel_id = cursor.fetchone()[0]
                        cursor.execute("COMMIT;")

                        self.print_(1, f"Uploading URL file: {in_name}")
                        with LineReader(self, in_path) as f:
                            for line in f.read_lines():
                                cursor.execute(
                                    SQL_UPLOAD_URL,
                                    (line, channel_id, line, channel_id, line))
                                cursor.execute("COMMIT;")
        except EOFError as e:
            self.print_(0, e)


if __name__ == "__main__":
    service = Application()
    service.run()

