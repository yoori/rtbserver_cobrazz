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
import flask
import werkzeug
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


class Metrics:
    def __init__(self):
        self.uploaded_users = 0
        self.files_to_upload = 0


class UploadDir:
    def __init__(self, in_dir, markers_dir, channel_prefix):
        self.in_dir = in_dir
        self.markers_dir = markers_dir
        self.channel_prefix = channel_prefix
        for root, dirs, files in os.walk(self.in_dir, True):
            self.files_to_upload = len(files)


class Upload(UploadDir):
    def __init__(self, service, params):
        workspace_dir = params["workspace_dir"]
        super().__init__(params["in_dir"], os.path.join(workspace_dir, "markers"), params["channel_prefix"])
        self.service = service
        self.account_id = params["account_id"]
        self.url_segments_dir = params.get("url_segments_dir")
        if self.url_segments_dir is not None:
            self.url_markers_dir = os.path.join(workspace_dir, "url_markers")
        self.__subdirs = {}
        self.metrics = Metrics()
        self.metrics_dirty = True

    def get_items(self):
        with self.service.lock:
            r = []
            if self.channel_prefix is not None:
                r.append(self)
            for root, dirs, files in os.walk(self.in_dir, True):
                dirs = set(dirs)
                for d in dirs:
                    try:
                        subdir = self.__subdirs[d]
                    except KeyError:
                        subdir = UploadDir(
                            os.path.join(self.in_dir, subdir),
                            os.path.join(self.markers_dir, subdir),
                            subdir
                        )
                        self.__subdirs[d] = subdir
                for d in tuple(self.__subdirs.keys()):
                    if d not in dirs:
                        del self.__subdirs[d]
                break
            r.extend(self.__subdirs.values())
            return r

    def get_metrics(self):
        with self.service.lock:
            if self.metrics_dirty:
                self.metrics.files_to_upload = self.files_to_upload
                for subdir in self.__subdirs.values():
                    self.metrics.files_to_upload += subdir.files_to_upload
                self.metrics_dirty = False
            return self.metrics


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
        self.name = basename + ext
        self.need_sign_uids = not signed_uids and ext in ("", ".txt", ".uids")

    def __eq__(self, other):
        return self.name == other.name and self.need_sign_uids == other.need_sign_uids and self.is_stable == other.is_stable


class HTTPThread(threading.Thread):
    def __init__(self, app, host, port):
        super().__init__()
        try:
            self.server = werkzeug.serving.make_server(host, port, app)
        except SystemExit as e:
            raise RuntimeError(f"Failed to make HTTP server: SystemExit {e}")
        self.ctx = app.app_context()
        self.ctx.push()

    def run(self):
        self.server.serve_forever()

    def shutdown(self):
        self.server.shutdown()
        self.join()


class Application(Service):
    def __init__(self):
        super().__init__()

        self.args_parser.add_argument("--upload-url", help="URL of server.")
        self.args_parser.add_argument("--upload-wait-time", type=float, help="Time to wait for upload.")
        self.args_parser.add_argument("--name", help="Upload name (for metrics).")
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
        self.args_parser.add_argument("--http-host", help="HTTP endpoint host.")
        self.args_parser.add_argument("--http-port", type=int, help="HTTP endpoint port.")

        self.lock = threading.Lock()
        self.__subprocesses = set()
        self.__asyncio_tasks = set()
        self.__http_thread = None

    def on_start(self):
        super().on_start()

        self.__upload_url = self.params["upload_url"]
        if isinstance(self.__upload_url, str):
            self.__upload_url = [self.__upload_url]
        self.__upload_wait_time = self.params["upload_wait_time"]

        self.__uploads = {}

        def add_upload(params):
            name = params["name"]
            if name in self.__uploads:
                raise RuntimeError(f"Upload name duplication: {name}")
            self.__uploads[name] = Upload(self, params)

        if self.params.get("account_id") is not None:
            add_upload(self.params)
        for upload in self.config.get("uploads", tuple()):
            add_upload(upload)

        self.__upload_threads = self.params["upload_threads"]

        self.__private_key_file = self.params["private_key_file"]

        if shutil.which("UserIdUtil") is None:
            raise RuntimeError("UserIdUtil not found")

        http_host = self.params.get("http_host")
        if http_host is not None:
            # initialize metrics before HTTP start
            for upload in self.__uploads.values():
                upload.get_items()
            self.__http_thread = HTTPThread(self.__make_http_application(), http_host, self.params["http_port"])
            self.print_(0, "Starting HTTP server")
            self.__http_thread.start()

    def on_stop_signal(self):
        with self.lock:
            self.print_(0, "Cancelling asyncio tasks...")
            for task in tuple(self.__asyncio_tasks):
                task.cancel()
                self.__asyncio_tasks.remove(task)
            self.print_(0, "Shutting down subprocesses...")
            for pid in self.__subprocesses:
                parent = psutil.Process(pid=pid)
                children = parent.children(recursive=True)
                children.append(parent)
                for p in children:
                    try:
                        p.kill()
                    except psutil.NoSuchProcess:
                        pass
        super().on_stop_signal()

    def on_stop(self):
        if self.__http_thread is not None:
            self.print_(0, "Shutting down HTTP server")
            self.__http_thread.shutdown()
            self.__http_thread = None
        super().on_stop()

    def on_timer(self):
        threads = []
        for upload in self.__uploads.values():
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
        pg_connection = None
        try:
            pg_connection = psycopg2.connect(
                f"host='{ph_host}' dbname='{pg_db}' user='{pg_user}' password='{pg_pass}'")
            with pg_connection.cursor() as pg_cursor:
                loop.run_until_complete(self.on_uids(upload, pg_cursor))
                loop.run_until_complete(self.on_urls(upload, pg_cursor))
        except psycopg2.Error as e:
            self.print_(0, e)
        except StopService:
            pass
        finally:
            if pg_connection is not None:
                pg_connection.close()
            loop.run_until_complete(asyncio.sleep(0.250))
            loop.close()

    async def on_uids(self, upload, pg_cursor):
        for item in upload.get_items():
            await self.on_uids_dir(pg_cursor, upload, item)

    async def on_uids_dir(self, pg_cursor, upload, item):
        self.print_(1, f"In dir {item.in_dir}")
        self.print_(1, f"Markers dir {item.markers_dir}")
        try:
            with Context(self, in_dir=item.in_dir) as in_dir_ctx:
                while True:
                    self.verify_running()
                    with Context(self, markers_dir=item.markers_dir) as markers_ctx:
                        in_names_all = tuple(in_dir_ctx.files.get_in_names())
                        with self.lock:
                            upload.metrics_dirty = True
                            item.files_to_upload = len(in_names_all)
                        if not in_names_all:
                            break
                        file_info = FileInfo(max(
                            in_names_all,
                            key=lambda in_name: os.path.getmtime(os.path.join(item.in_dir, in_name))))
                        reg_marker_name = file_info.name + ".__reg__"
                        keyword = make_keyword(item.channel_prefix.lower() + file_info.name.lower())
                        if markers_ctx.markers.add(reg_marker_name):
                            channel_id = item.channel_prefix + file_info.name.upper()
                            self.print_(1, f"Registering file: {reg_marker_name} ({file_info.name}) channel_id {channel_id} account_id {item.account_id} keyword {keyword}")
                            pg_cursor.execute(
                                SQL_REG_USER,
                                (channel_id, item.account_id, keyword, keyword, keyword, keyword, keyword))
                            pg_cursor.execute("COMMIT;")

                        if not markers_ctx.markers.check_mtime_interval(reg_marker_name, self.__upload_wait_time):
                            continue

                        in_names = []
                        for in_name in in_names_all:
                            if file_info != FileInfo(in_name):
                                continue
                            in_names.append(in_name)
                        in_names.sort(
                            key=lambda _: os.path.getmtime(os.path.join(item.in_dir, _)),
                            reverse=True)
                        in_names = in_names[:12]

                        for in_name_index, in_name in enumerate(in_names):
                            if in_name_index == 0:
                                self.print_(1, f"Processing file group: name={file_info.name} need_sign_uids={file_info.need_sign_uids} is_stable={file_info.is_stable}")
                            self.print_(1, f" {in_name_index + 1}) File {in_name}")

                        if len(in_names) > 1:
                            file_groups = {
                                0: dict(
                                    (in_name, open(os.path.join(item.in_dir, in_name), "rb"))
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
                                                os.remove(os.path.join(item.in_dir, in_name))
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

                        self.print_(0, f"Unique files count: {len(in_names)}")

                        async def run_lines(f):
                            async with aiohttp.ClientSession() as session:
                                tasks = []
                                loop = asyncio.get_event_loop()
                                try:
                                    with self.lock:
                                        for i in range(self.__upload_threads):
                                            task = loop.create_task(
                                                self.on_uids_lines(f, file_info.is_stable, keyword, session, upload))
                                            tasks.append(task)
                                            self.__asyncio_tasks.add(task)
                                    for task in tasks:
                                        try:
                                            await task
                                        except asyncio.CancelledError:
                                            pass
                                finally:
                                    with self.lock:
                                        for task in tasks:
                                            self.__asyncio_tasks.discard(task)

                        in_paths_str = " ".join(os.path.join(item.in_dir, in_name) for in_name in in_names)

                        if file_info.need_sign_uids:
                            cmd = ['sh', '-c', f'cat {in_paths_str} | sed -r "s/([^.])$/\\1../" | sort -u --parallel=4 | UserIdUtil sign-uid --private-key-file="{self.__private_key_file}"']
                        else:
                            cmd = ['sh', '-c', f'cat {in_paths_str} | sort -u --parallel=4']

                        with subprocess.Popen(cmd, stdout=subprocess.PIPE) as shp:
                            with self.lock:
                                self.__subprocesses.add(shp.pid)
                            try:
                                file = io.TextIOWrapper(io.BufferedReader(shp.stdout, buffer_size=65536), encoding="utf-8")
                                self.print_(0, f"Processing...")
                                with LineReader(self, path=file_info.name, file=file) as f:
                                    await run_lines(f)
                            finally:
                                with self.lock:
                                    self.__subprocesses.remove(shp.pid)

                        self.verify_running()
                        if shp.returncode >= 0:
                            for in_name in in_names:
                                self.print_(1, f"Done file: {in_name}")
                                os.remove(os.path.join(item.in_dir, in_name))

        except aiohttp.client_exceptions.ClientError as e:
            self.print_(0, e)
        except EOFError as e:
            self.print_(0, e)

    async def on_uids_lines(self, f, is_stable, keyword, session, upload):

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

                    async def visitor(resp):
                        uid = resp.cookies.get("uid")
                        if uid is not None:
                            await get(uid.value)

                    await self.request(
                        session,
                        path="track.gif",
                        headers={},
                        params={"xid": "megafon-stableid/" + line, "u": "yUeKE9yKRKSu3bhliRyREA.."},
                        visitor=visitor)
                with self.lock:
                    upload.metrics.uploaded_users += 1
        except StopService:
            pass

    async def request(self, session, path, headers, params, visitor=None):
        url = f"{random.choice(self.__upload_url)}/{path}"
        self.print_(3, f"Request url={url} headers={headers} params={params}")
        try:
            async with session.get(url=url, params=params, headers=headers, ssl=False) as resp:
                if resp.status != 204:
                    raise aiohttp.client_exceptions.ClientResponseError(resp.request_info, resp.history)
                if visitor is not None:
                    await visitor(resp)
        except aiohttp.client_exceptions.ClientError as e:
            self.print_(0, e)

    async def on_urls(self, upload, pg_cursor):
        if upload.url_segments_dir is None:
            return

        try:
            with Context(self, in_dir=upload.url_segments_dir, markers_dir=upload.url_markers_dir) as ctx:
                for in_name in ctx.files.get_in_names():
                    in_path = os.path.join(ctx.in_dir, in_name)
                    if ctx.markers.add(in_name, os.path.getmtime(in_path)):
                        self.print_(1, f"Registering URL file: {in_name}")
                        pg_cursor.execute(SQL_REG_URL, (upload.channel_prefix + in_name.upper(),))
                        channel_id = pg_cursor.fetchone()[0]
                        pg_cursor.execute("COMMIT;")
                        self.print_(1, f"Uploading URL file: {in_name}")
                        with LineReader(self, in_path) as f:
                            for line in f.read_lines():
                                pg_cursor.execute(SQL_UPLOAD_URL, (line, channel_id, line, channel_id, line))
                                pg_cursor.execute("COMMIT;")
        except EOFError as e:
            self.print_(0, e)

    def __make_http_application(self):
        app = flask.Flask('SegmentUploaderHTTP')

        @app.route('/metrics')
        def metrics():
            mm = dict(
                (name, upload.get_metrics())
                for name, upload in self.__uploads.items())
            fmt = flask.request.args.get('format')
            if fmt is None:
                prometheus = []
                for name, m in mm.items():
                    prometheus.append(('files_to_upload', name, m.files_to_upload))
                for name, m in mm.items():
                    prometheus.append(('uploaded_users', name, m.uploaded_users))
                return "\n".join(
                    '{}{{source = "{}"}} {}'.format(*p)
                    for p in prometheus
                )
            if fmt == "json":
                files_to_upload = []
                uploaded_users = []
                for name, m in mm.items():
                    files_to_upload.append({"source": name, "value": m.files_to_upload})
                    uploaded_users.append({"source": name, "value": m.uploaded_users})
                return flask.jsonify({"files_to_upload": files_to_upload, "uploaded_users": uploaded_users})
            return "Unknown format", 400

        return app


if __name__ == "__main__":
    service = Application()
    service.run()

