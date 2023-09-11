#!/usr/bin/python3

import os
from datetime import datetime
import clickhouse_driver
import clickhouse_driver.errors
from ServiceUtilsPy.Service import Service
from ServiceUtilsPy.Context import Context
from ServiceUtilsPy.LineIO import LineReader


class Upload:
    def __init__(self, service, params, table_name, csv_header):
        self.service = service
        self.__in_dir = params["in_dir"]
        self.__batch_size = params.get("batch_size", service.batch_size)
        self.__table_name = table_name
        self.__csv_header = csv_header
        self.__insert_sql = f"INSERT INTO {table_name}({csv_header}) VALUES"

    def process(self):
        values = []

        def flush():
            if values:
                self.service.ch_client.execute(self.__insert_sql, values)
                values.clear()

        with Context(self.service, in_dir=self.__in_dir) as ctx:
            for in_file in ctx.files.get_in_files():
                self.service.print_(0, f"Processing {in_file}")
                in_path = os.path.join(ctx.in_dir, in_file)
                with LineReader(self.service, path=in_path) as f:
                    if f.read_line(progress=False) != self.__csv_header:
                        raise RuntimeError(f"{self.__table_name} header mismatch")
                    for line in f.read_lines():
                        values.append(self.on_line_to_value(line))
                        if len(values) >= self.__batch_size:
                            flush()
                    flush()
                self.service.print_(0, f"Removing {in_file}")
                os.remove(in_path)

    def on_line_to_value(self, line):
        raise NotImplementedError


class RequestStatsHourlyExtStatUpload(Upload):
    def __init__(self, service, params):
        super().__init__(
            service,
            params,
            "RequestStatsHourlyExtStat",
            "sdate,adv_sdate,colo_id,publisher_account_id,tag_id,size_id,country_code,adv_account_id,campaign_id,"
            "ccg_id,cc_id,ccg_rate_id,colo_rate_id,site_rate_id,currency_exchange_id,delivery_threshold,num_shown,"
            "position,test,fraud,walled_garden,user_status,geo_channel_id,device_channel_id,ctr_reset_id,hid_profile,"
            "viewability,unverified_imps,imps,clicks,actions,adv_amount,pub_amount,isp_amount,adv_comm_amount,"
            "pub_comm_amount,adv_payable_comm_amount,pub_advcurrency_amount,isp_advcurrency_amount,undup_imps,"
            "undup_clicks,ym_confirmed_clicks,ym_bounced_clicks,ym_robots_clicks,ym_session_time"
        )

    def on_line_to_value(self, line):
        (sdate, adv_sdate, colo_id, publisher_account_id, tag_id, size_id, country_code, adv_account_id, campaign_id,
         ccg_id, cc_id, ccg_rate_id, colo_rate_id, site_rate_id,  currency_exchange_id, delivery_threshold, num_shown,
         position, test, fraud, walled_garden, user_status, geo_channel_id, device_channel_id, ctr_reset_id,
         hid_profile, viewability, unverified_imps, imps, clicks, actions, adv_amount, pub_amount, isp_amount,
         adv_comm_amount, pub_comm_amount, adv_payable_comm_amount, pub_advcurrency_amount, isp_advcurrency_amount,
         undup_imps, undup_clicks, ym_confirmed_clicks, ym_bounced_clicks, ym_robots_clicks, ym_session_time
         ) = line.split(",")
        return [
            datetime.strptime(sdate, "%Y-%m-%d %H:%M:%S"), datetime.strptime(adv_sdate, "%Y-%m-%d %H:%M:%S"),
            int(colo_id), int(publisher_account_id), int(tag_id), int(size_id) if size_id else None, country_code,
            int(adv_account_id), int(campaign_id), int(ccg_id), int(cc_id), int(ccg_rate_id), int(colo_rate_id),
            int(site_rate_id), int(currency_exchange_id), delivery_threshold, int(num_shown), int(position),
            bool(int(test)), bool(int(fraud)), bool(int(walled_garden)), user_status, int(geo_channel_id) if
            geo_channel_id else None, int(device_channel_id) if device_channel_id else None, int(ctr_reset_id),
            bool(int(hid_profile)), int(viewability), int(unverified_imps), int(imps), int(clicks), int(actions),
            adv_amount, pub_amount, isp_amount, adv_comm_amount, pub_comm_amount, adv_payable_comm_amount,
            pub_advcurrency_amount, isp_advcurrency_amount, int(undup_imps), int(undup_clicks),
            int(ym_confirmed_clicks), int(ym_bounced_clicks), int(ym_robots_clicks), ym_session_time
        ]


UPLOAD_TYPES = {
    "RequestStatsHourlyExtStat": RequestStatsHourlyExtStatUpload
}


class Application(Service):
    def __init__(self):
        super().__init__()
        self.args_parser.add_argument("--ch-host", help="Clickhouse hostname.")
        self.args_parser.add_argument("--batch-size", type=int, default=100, help="SQL batch size (for single upload).")
        self.args_parser.add_argument("--upload-type", help="Upload type (for single upload).")
        self.ch_client = None
        self.__uploads = []

    def on_start(self):
        super().on_start()

        self.batch_size = self.params["batch_size"]

        def add_upload(params):
            upload_type_name = params["upload_type"]
            try:
                upload_type = UPLOAD_TYPES[upload_type_name]
            except KeyError:
                raise RuntimeError(f"Upload type {upload_type_name} not found")
            self.__uploads.append(upload_type(self, params))

        if self.params.get("upload_type") is not None:
            add_upload(self.params)
        for upload in self.config.get("uploads", tuple()):
            add_upload(upload)

    def on_timer(self):
        try:
            if self.ch_client is None:
                self.ch_client = clickhouse_driver.Client(host=self.params["ch_host"])
            for upload in self.__uploads:
                upload.process()
        except clickhouse_driver.errors.Error as e:
            self.print_(0, e)
            self.__close_ch_client()

    def on_stop(self):
        self.__close_ch_client()
        super().on_stop()

    def __close_ch_client(self):
        if self.ch_client is not None:
            self.ch_client.disconnect()
            self.ch_client = None


if __name__ == "__main__":
    service = Application()
    service.run()

