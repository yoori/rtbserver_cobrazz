#! /usr/bin/env python3

import os
import io
import asyncio
import aiohttp
import aiohttp.client_exceptions
import psycopg2
import subprocess
import asyncio
import logging
import threading
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

    def on_start(self):
        super().on_start()

        self.upload_url = self.params["upload_url"]
        self.upload_wait_time = self.params["upload_wait_time"]

        self.uploads = []
        if self.args.account_id is not None:
            self.uploads.append(Upload(self.params))
        for upload in self.config.get("uploads", tuple()):
            self.uploads.append(Upload(upload))

        self.upload_threads = self.params["upload_threads"]

        self.private_key_file = self.params["private_key_file"]

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
        except psycopg2.Error:
            logging.error("Postgre error")
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
        try:
            with Context(self, in_dir=in_dir, markers_dir=markers_dir) as ctx:
                for in_name in ctx.files.get_in_files():
                    in_path = os.path.join(in_dir, in_name)

                    signed_uids = False
                    basename, ext = os.path.splitext(in_name)
                    if ext.startswith(".stamp"):
                        basename, ext = os.path.splitext(basename)
                    if ext.startswith(".signed_uids"):
                        basename, ext = os.path.splitext(basename)
                        signed_uids = True
                    if ext in (".stable", ".uids", ".txt"):
                        basename, ext = os.path.splitext(basename)
                    fname = basename + ext

                    reg_marker_name = fname + ".__reg__"
                    upload_marker_name = in_name + ".__upload__"
                    keyword = make_keyword(channel_prefix.lower() + basename.lower())

                    if ctx.markers.add(reg_marker_name):
                        self.print_(1, f"Registering file: {reg_marker_name} ({in_name})")
                        cursor.execute(
                            SQL_REG_USER,
                            (channel_prefix + basename.upper(), account_id, keyword, keyword, keyword, keyword, keyword))
                        cursor.execute("COMMIT;")
                        continue

                    if ctx.markers.check_mtime_interval(reg_marker_name, self.upload_wait_time):
                        if ctx.markers.add(upload_marker_name, os.path.getmtime(in_path)):
                            self.print_(1, f"Uploading file: {upload_marker_name} ({in_name})")
                            is_stable = (ext == ".stable")

                            async def run_lines(f):
                                await asyncio.gather(
                                    *tuple(self.on_line(f, is_stable, keyword) for i in range(self.upload_threads)))

                            if not signed_uids and ext in ("", ".txt", ".uids"):
                                with subprocess.Popen(
                                    ['sh', '-c', f'cat "{in_path}" | sed -r "s/([^.])$/\\1../" | UserIdUtil sign-uid --private-key-file="{self.private_key_file}"'],
                                        stdout=subprocess.PIPE) as shp:
                                    file = io.TextIOWrapper(io.BufferedReader(shp.stdout, buffer_size=65536), encoding="utf-8")
                                    with LineReader(self, path=in_path, file=file) as f:
                                        await run_lines(f)
                            else:
                                with LineReader(self, in_path) as f:
                                    await run_lines(f)

        except aiohttp.client_exceptions.ClientError:
            logging.error("aiohttp error")
        except EOFError:
            logging.error("EOFError error")

    async def on_line(self, f, is_stable, keyword):

        async def get(uid):
            return await self.request(
                path="get",
                headers={"Host": "ad.new-programmatic.com", "Cookie": "uid=" + uid},
                params={"loc.name": "ru", "referer-kw": keyword})

        for line in f.read_lines():
            if not is_stable:
                await get(line)
            else:

                async def visitor(session, resp):
                    uid = resp.cookies.get("uid")
                    if uid is not None:
                        await get(uid.value)

                await self.request(
                    path="track.gif",
                    headers={},
                    params={"xid": "megafon-stableid/" + line, "u": "yUeKE9yKRKSu3bhliRyREA.."},
                    visitor=visitor)

    async def request(self, path, headers, params, visitor=None):
        url = f"{self.upload_url}/{path}"
        self.print_(3, "request ", "url=", url, "headers=", headers, "params=", params)
        async with aiohttp.ClientSession(headers=headers) as session:
            async with session.get(url=url, params=params, ssl=False) as resp:
                if resp.status != 204:
                    raise aiohttp.client_exceptions.ClientResponseError
                if visitor is not None:
                    await visitor(session, resp)

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
        except EOFError:
            logging.error("EOFError error")


if __name__ == "__main__":
    service = Application()
    service.run()


