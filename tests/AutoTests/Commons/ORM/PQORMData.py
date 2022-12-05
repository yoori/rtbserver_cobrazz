from ORMBase import *

account = Object('account', 'Account', True)
account.id = PQIndex([ORMInt('account_id', 'account_id')], 'account_account_id_seq')
account.fields = [
    ORMInt          ('account_manager_id',            'account_manager_id'),
    ORMInt          ('account_type_id',               'account_type_id', 'accounttype'),
    ORMInt          ('adv_contact_id',                'adv_contact_id'),
    ORMInt          ('agency_account_id',             'agency_account_id'),
    ORMInt          ('billing_address_id',            'billing_address_id'),
    ORMString       ('business_area',                 'business_area'),
    ORMInt          ('cmp_contact_id',                'cmp_contact_id'),
    ORMString       ('company_registration_number',   'company_registration_number'),
    ORMString       ('contact_name',                  'contact_name'),
    ORMString       ('country_code',                  'country_code', 'country'),
    ORMInt          ('currency_id',                   'currency', 'currency'),
    ORMInt          ('display_status_id',             'display_status_id'),
    ORMInt          ('flags',                         'flags'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMInt          ('internal_account_id',           'internal_account_id'),
    ORMInt          ('isp_contact_id',                'isp_contact_id'),
    ORMTimestamp    ('last_deactivated',              'last_deactivated'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('legal_address_id',              'legal_address_id'),
    ORMString       ('legal_name',                    'legal_name'),
    ORMFloat        ('message_sent',                  'message_sent'),
    ORMString       ('name',                          'name'),
    ORMString       ('notes',                         'notes'),
    ORMBool         ('passback_below_fold',           'passback_below_fold'),
    ORMInt          ('pub_contact_id',                'pub_contact_id'),
    ORMString       ('pub_pixel_optin',               'pub_pixel_optin'),
    ORMString       ('pub_pixel_optout',              'pub_pixel_optout'),
    ORMInt          ('role_id',                       'role_id'),
    ORMString       ('specific_business_area',        'specific_business_area'),
    ORMString       ('status',                        'status'),
    ORMString       ('text_adserving',                'text_adserving'),
    ORMInt          ('timezone_id',                   'timezone_id'),
    ORMBool         ('use_pub_pixel',                 'use_pub_pixel'),
    ORMTimestamp    ('version',                       'version'),
  ]

accountaddress = Object('accountaddress', 'Accountaddress', True)
accountaddress.id = PQIndex([ORMInt('address_id', 'address_id')], 'accountaddress_address_id_seq')
accountaddress.fields = [
    ORMString       ('city',                          'city'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('line1',                         'line1'),
    ORMString       ('line2',                         'line2'),
    ORMString       ('line3',                         'line3'),
    ORMString       ('province',                      'province'),
    ORMString       ('state',                         'state'),
    ORMTimestamp    ('version',                       'version'),
    ORMString       ('zip',                           'zip'),
  ]

accountfinancialdata = Object('accountfinancialdata', 'Accountfinancialdata', True)
accountfinancialdata.id = PQIndex([ORMInt('account_id', 'account_id')])
accountfinancialdata.fields = [
    ORMFloat        ('camp_credit_used',              'camp_credit_used'),
    ORMFloat        ('invoiced_outstanding',          'invoiced_outstanding'),
    ORMFloat        ('invoiced_received',             'invoiced_received'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('not_invoiced',                  'not_invoiced'),
    ORMFloat        ('payments_billed',               'payments_billed'),
    ORMFloat        ('payments_paid',                 'payments_paid'),
    ORMFloat        ('payments_unbilled',             'payments_unbilled'),
    ORMFloat        ('prepaid_amount',                'prepaid_amount'),
    ORMFloat        ('total_adv_amount',              'total_adv_amount'),
    ORMFloat        ('total_paid',                    'total_paid'),
    ORMString       ('type',                          'type'),
    ORMFloat        ('unbilled_schedule_of_works',    'unbilled_schedule_of_works'),
    ORMTimestamp    ('version',                       'version'),
  ]

accountfinancialsettings = Object('accountfinancialsettings', 'Accountfinancialsettings', False)
accountfinancialsettings.id = PQIndex([ORMInt('account_id', 'account_id')])
accountfinancialsettings.fields = [
    ORMString       ('bank_account_check_digits',     'bank_account_check_digits'),
    ORMInt          ('bank_account_currency_id',      'bank_account_currency_id'),
    ORMString       ('bank_account_iban',             'bank_account_iban'),
    ORMString       ('bank_account_name',             'bank_account_name'),
    ORMString       ('bank_account_number',           'bank_account_number'),
    ORMString       ('bank_account_type',             'bank_account_type'),
    ORMString       ('bank_bic_code',                 'bank_bic_code'),
    ORMString       ('bank_branch_name',              'bank_branch_name'),
    ORMString       ('bank_country_code',             'bank_country_code'),
    ORMString       ('bank_name',                     'bank_name'),
    ORMString       ('bank_number',                   'bank_number'),
    ORMString       ('bank_sort_code',                'bank_sort_code'),
    ORMString       ('billing_frequency',             'billing_frequency'),
    ORMFloat        ('billing_frequency_offset',      'billing_frequency_offset'),
    ORMFloat        ('commission',                    'commission'),
    ORMFloat        ('credit_limit',                  'credit_limit'),
    ORMInt          ('default_bill_to_user_id',       'default_bill_to_user_id'),
    ORMFloat        ('flags',                         'flags'),
    ORMString       ('insurance_number',              'insurance_number'),
    ORMBool         ('is_frozen',                     'is_frozen'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('min_invoice',                   'min_invoice'),
    ORMTimestamp    ('on_account_credit_updated',     'on_account_credit_updated'),
    ORMString       ('payment_method',                'payment_method'),
    ORMString       ('payment_terms',                 'payment_terms'),
    ORMString       ('tax_notes',                     'tax_notes'),
    ORMString       ('tax_number',                    'tax_number'),
    ORMFloat        ('tax_rate',                      'tax_rate'),
    ORMString       ('type',                          'type'),
    ORMTimestamp    ('version',                       'version'),
  ]

accountrole = Object('accountrole', 'Accountrole', True)
accountrole.id = PQIndex([ORMInt('account_role_id', 'account_role_id')])
accountrole.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
  ]

accounttype = Object('accounttype', 'Accounttype', True)
accounttype.id = PQIndex([ORMInt('account_type_id', 'account_type_id')], 'accounttype_account_type_id_seq')
accounttype.fields = [
    ORMInt          ('account_role_id',               'account_role_id', 'accountrole'),
    ORMString       ('adv_exclusion_approval',        'adv_exclusion_approval'),
    ORMString       ('adv_exclusions',                'adv_exclusions'),
    ORMString       ('auction_rate',                  'auction_rate'),
    ORMFloat        ('campaign_check_1',              'campaign_check_1'),
    ORMFloat        ('campaign_check_2',              'campaign_check_2'),
    ORMFloat        ('campaign_check_3',              'campaign_check_3'),
    ORMBool         ('campaign_check_on',             'campaign_check_on'),
    ORMFloat        ('channel_check_1',               'channel_check_1'),
    ORMFloat        ('channel_check_2',               'channel_check_2'),
    ORMFloat        ('channel_check_3',               'channel_check_3'),
    ORMBool         ('channel_check_on',              'channel_check_on'),
    ORMInt          ('flags',                         'flags'),
    ORMBool         ('io_management',                 'io_management'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('max_keyword_length',            'max_keyword_length'),
    ORMFloat        ('max_keywords_per_channel',      'max_keywords_per_channel'),
    ORMFloat        ('max_keywords_per_group',        'max_keywords_per_group'),
    ORMFloat        ('max_url_length',                'max_url_length'),
    ORMFloat        ('max_urls_per_channel',          'max_urls_per_channel'),
    ORMBool         ('mobile_operator_targeting',     'mobile_operator_targeting'),
    ORMString       ('name',                          'name'),
    ORMBool         ('show_browser_passback_tag',     'show_browser_passback_tag'),
    ORMBool         ('show_iframe_tag',               'show_iframe_tag'),
    ORMTimestamp    ('version',                       'version'),
  ]

accounttypecreativesize = Object('accounttypecreativesize', 'Accounttypecreativesize', False)
accounttypecreativesize.id = PQIndex([
    ORMInt          ('account_type_id',               'account_type_id'),
    ORMInt          ('size_id',                       'size_id'),
  ] )
accounttypecreativesize.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

accounttypecreativetemplate = Object('accounttypecreativetemplate', 'Accounttypecreativetemplate', False)
accounttypecreativetemplate.id = PQIndex([
    ORMInt          ('account_type_id',               'account_type_id'),
    ORMInt          ('template_id',                   'template_id'),
  ] )
accounttypecreativetemplate.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

accounttypedevicechannel = Object('accounttypedevicechannel', 'Accounttypedevicechannel', False)
accounttypedevicechannel.id = PQIndex([
    ORMInt          ('account_type_id',               'account_type_id'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
  ] )
accounttypedevicechannel.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

accounttype_ccgtype = Object('accounttype_ccgtype', 'Accounttype_ccgtype', False)
accounttype_ccgtype.id = PQIndex([ORMInt('accounttype_ccgtype_id', 'accounttype_ccgtype_id')], 'accounttype_ccgtype_accounttype_ccgtype_id_seq')
accounttype_ccgtype.fields = [
    ORMInt          ('account_type_id',               'account_type_id', 'accounttype'),
    ORMString       ('ccg_type',                      'ccg_type'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('rate_type',                     'rate_type'),
    ORMString       ('tgt_type',                      'tgt_type'),
  ]

action = Object('action', 'Action', True)
action.id = PQIndex([ORMInt('action_id', 'action_id')], 'action_action_id_seq')
action.fields = [
    ORMInt          ('account_id',                    'account_id', 'account'),
    ORMFloat        ('click_window',                  'click_window', default = 30),
    ORMInt          ('conv_category_id',              'conv_category_id', default = "0"),
    ORMFloat        ('cur_value',                     'cur_value', default = "0"),
    ORMInt          ('display_status_id',             'display_status_id'),
    ORMFloat        ('imp_window',                    'imp_window', default = 7),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('status',                        'status'),
    ORMString       ('url',                           'url'),
    ORMTimestamp    ('version',                       'version'),
  ]

actionrequests = Object('actionrequests', 'Actionrequests', False)
actionrequests.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('action_date',                   'action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
  ] )
actionrequests.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('country_action_hour',           'country_action_hour'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ]

actionrequestsdaily = Object('actionrequestsdaily', 'Actionrequestsdaily', False)
actionrequestsdaily.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ] )
actionrequestsdaily.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequestsdaily_country_action_date_y2014m10 = Object('actionrequestsdaily_country_action_date_y2014m10', 'Actionrequestsdaily_country_action_date_y2014m10', False)
actionrequestsdaily_country_action_date_y2014m10.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ] )
actionrequestsdaily_country_action_date_y2014m10.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequestsdaily_country_action_date_y2015m04 = Object('actionrequestsdaily_country_action_date_y2015m04', 'Actionrequestsdaily_country_action_date_y2015m04', False)
actionrequestsdaily_country_action_date_y2015m04.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ] )
actionrequestsdaily_country_action_date_y2015m04.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequestsmonthly = Object('actionrequestsmonthly', 'Actionrequestsmonthly', False)
actionrequestsmonthly.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ] )
actionrequestsmonthly.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequestsmonthly_country_action_date_y2014m10 = Object('actionrequestsmonthly_country_action_date_y2014m10', 'Actionrequestsmonthly_country_action_date_y2014m10', False)
actionrequestsmonthly_country_action_date_y2014m10.id = PQIndex([
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMDate         ('country_action_date',           'country_action_date'),
  ] )
actionrequestsmonthly_country_action_date_y2014m10.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequestsmonthly_country_action_date_y2015m04 = Object('actionrequestsmonthly_country_action_date_y2015m04', 'Actionrequestsmonthly_country_action_date_y2015m04', False)
actionrequestsmonthly_country_action_date_y2015m04.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ] )
actionrequestsmonthly_country_action_date_y2015m04.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequeststotal = Object('actionrequeststotal', 'Actionrequeststotal', False)
actionrequeststotal.id = PQIndex([ORMInt('action_id', 'action_id')])
actionrequeststotal.fields = [
    ORMTimestamp    ('last_action_date',              'last_action_date'),
  ]

actionrequests_country_action_date_y2014w43 = Object('actionrequests_country_action_date_y2014w43', 'Actionrequests_country_action_date_y2014w43', False)
actionrequests_country_action_date_y2014w43.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('action_date',                   'action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ] )
actionrequests_country_action_date_y2014w43.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('country_action_hour',           'country_action_hour'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
  ]

actionrequests_country_action_date_y2015w16 = Object('actionrequests_country_action_date_y2015w16', 'Actionrequests_country_action_date_y2015w16', False)
actionrequests_country_action_date_y2015w16.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('action_date',                   'action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
  ] )
actionrequests_country_action_date_y2015w16.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('country_action_hour',           'country_action_hour'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ]

actionrequests_country_action_date_y2015w17 = Object('actionrequests_country_action_date_y2015w17', 'Actionrequests_country_action_date_y2015w17', False)
actionrequests_country_action_date_y2015w17.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('action_date',                   'action_date'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMString       ('user_status',                   'user_status'),
  ] )
actionrequests_country_action_date_y2015w17.fields = [
    ORMFloat        ('action_request_count',          'action_request_count'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('country_action_hour',           'country_action_hour'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMFloat        ('cur_value',                     'cur_value'),
  ]

actionstats = Object('actionstats', 'Actionstats', False)
actionstats.id = PQIndex([
    ORMString       ('action_request_id',             'action_request_id'),
    ORMString       ('request_id',                    'request_id'),
  ] )
actionstats.fields = [
    ORMTimestamp    ('action_date',                   'action_date'),
    ORMInt          ('action_id',                     'action_id'),
    ORMString       ('action_referrer_url',           'action_referrer_url'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMTimestamp    ('click_date',                    'click_date'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('country_action_hour',           'country_action_hour'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMString       ('country_code',                  'country_code'),
    ORMFloat        ('cur_value',                     'cur_value'),
    ORMTimestamp    ('imp_date',                      'imp_date'),
    ORMString       ('order_id',                      'order_id'),
    ORMInt          ('tag_id',                        'tag_id'),
  ]

actiontype = Object('actiontype', 'Actiontype', False)
actiontype.id = PQIndex([ORMInt('action_type_id', 'action_type_id')])
actiontype.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
  ]

adsconfig = Object('adsconfig', 'Adsconfig', False)
adsconfig.id = PQIndex([ORMString('param_name', 'param_name')])
adsconfig.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('param_value',                   'param_value'),
    ORMTimestamp    ('version',                       'version'),
  ]

advertiserdevicestatsdaily = Object('advertiserdevicestatsdaily', 'Advertiserdevicestatsdaily', False)
advertiserdevicestatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
  ] )
advertiserdevicestatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('pub_amount_adv',                'pub_amount_adv'),
    ORMInt          ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

advertiserstatsdaily = Object('advertiserstatsdaily', 'Advertiserstatsdaily', False)
advertiserstatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
  ] )
advertiserstatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('pub_amount_adv',                'pub_amount_adv'),
    ORMInt          ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

advertiserstatsdailycountry = Object('advertiserstatsdailycountry', 'Advertiserstatsdailycountry', False)
advertiserstatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('ccg_rate_id',                   'ccg_rate_id'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
  ] )
advertiserstatsdailycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMFloat        ('adv_invoice_comm_amount',       'adv_invoice_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('isp_amount_adv',                'isp_amount_adv'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMFloat        ('pub_amount_adv',                'pub_amount_adv'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

advertiserstatsmonthly = Object('advertiserstatsmonthly', 'Advertiserstatsmonthly', False)
advertiserstatsmonthly.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
  ] )
advertiserstatsmonthly.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('pub_amount_adv',                'pub_amount_adv'),
    ORMInt          ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

advertiserstatstotal = Object('advertiserstatstotal', 'Advertiserstatstotal', False)
advertiserstatstotal.id = PQIndex([
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
  ] )
advertiserstatstotal.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('requests',                      'requests'),
  ]

advertiseruserstats = Object('advertiseruserstats', 'Advertiseruserstats', False)
advertiseruserstats.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
advertiseruserstats.fields = [
    ORMFloat        ('display_unique_users',          'display_unique_users'),
    ORMFloat        ('text_unique_users',             'text_unique_users'),
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

advertiseruserstatsrunning = DualObject('advertiseruserstatsrunning', 'Advertiseruserstatsrunning', False)
advertiseruserstatsrunning.id = PQIndex([
  ] )
advertiseruserstatsrunning.fields = [
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMFloat        ('daily_display_unique_users',    'daily_display_unique_users'),
    ORMFloat        ('daily_text_unique_users',       'daily_text_unique_users'),
    ORMFloat        ('daily_unique_users',            'daily_unique_users'),
    ORMFloat        ('monthly_display_unique_users',  'monthly_display_unique_users'),
    ORMFloat        ('monthly_text_unique_users',     'monthly_text_unique_users'),
    ORMFloat        ('monthly_unique_users',          'monthly_unique_users'),
    ORMFloat        ('new_display_unique_users',      'new_display_unique_users'),
    ORMFloat        ('new_text_unique_users',         'new_text_unique_users'),
    ORMFloat        ('new_unique_users',              'new_unique_users'),
    ORMFloat        ('running_display_unique_users',  'running_display_unique_users'),
    ORMFloat        ('running_text_unique_users',     'running_text_unique_users'),
    ORMFloat        ('running_unique_users',          'running_unique_users'),
    ORMFloat        ('weekly_display_unique_users',   'weekly_display_unique_users'),
    ORMFloat        ('weekly_text_unique_users',      'weekly_text_unique_users'),
    ORMFloat        ('weekly_unique_users',           'weekly_unique_users'),
  ]

advertiseruserstatstotal = Object('advertiseruserstatstotal', 'Advertiseruserstatstotal', False)
advertiseruserstatstotal.id = PQIndex([ORMInt('adv_account_id', 'adv_account_id')])
advertiseruserstatstotal.fields = [
    ORMFloat        ('display_unique_users',          'display_unique_users'),
    ORMFloat        ('text_unique_users',             'text_unique_users'),
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

appformat = Object('appformat', 'Appformat', True)
appformat.id = PQIndex([ORMInt('app_format_id', 'app_format_id')], 'appformat_app_format_id_seq')
appformat.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('mime_type',                     'mime_type'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('version',                       'version'),
  ]

auctionsettings = Object('auctionsettings', 'Auctionsettings', False)
auctionsettings.id = PQIndex([ORMInt('account_id', 'account_id')], 'auctionsettings_account_id_seq')
auctionsettings.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('max_ecpm_share',                'max_ecpm_share'),
    ORMFloat        ('max_random_cpm',                'max_random_cpm'),
    ORMFloat        ('prop_probability_share',        'prop_probability_share'),
    ORMFloat        ('random_share',                  'random_share'),
    ORMTimestamp    ('version',                       'version'),
  ]

auditlog = Object('auditlog', 'Auditlog', False)
auditlog.id = PQIndex([ORMInt('log_id', 'log_id')], 'auditlog_log_id_seq')
auditlog.fields = [
    ORMString       ('action_descr',                  'action_descr'),
    ORMInt          ('action_type_id',                'action_type_id', 'actiontype'),
    ORMString       ('ip',                            'ip'),
    ORMInt          ('job_id',                        'job_id'),
    ORMTimestamp    ('log_date',                      'log_date'),
    ORMInt          ('object_account_id',             'object_account_id'),
    ORMInt          ('object_id',                     'object_id'),
    ORMInt          ('object_type_id',                'object_type_id', 'objecttype'),
    ORMBool         ('success',                       'success'),
    ORMInt          ('user_id',                       'user_id', 'users'),
  ]

authenticationtoken = Object('authenticationtoken', 'Authenticationtoken', False)
authenticationtoken.id = PQIndex([ORMString('token', 'token')])
authenticationtoken.fields = [
    ORMString       ('ip',                            'ip'),
    ORMFloat        ('last_update',                   'last_update'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('user_id',                       'user_id'),
  ]

behavioralparameters = Object('behavioralparameters', 'BehavioralParameters', True)
behavioralparameters.id = PQIndex([ORMInt('behav_params_id', 'behav_params_id')], 'behavioralparameters_behav_params_id_seq')
behavioralparameters.fields = [
    ORMInt          ('behav_params_list_id',          'behav_params_list_id', 'behavioralparameterslist'),
    ORMInt          ('channel_id',                    'channel', 'channel'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('minimum_visits',                'minimum_visits'),
    ORMFloat        ('time_from',                     'time_from'),
    ORMFloat        ('time_to',                       'time_to'),
    ORMString       ('trigger_type',                  'trigger_type'),
    ORMTimestamp    ('version',                       'version'),
    ORMFloat        ('weight',                        'weight', default = 1),
  ]
behavioralparameters.asks = [
    Select('channel', ['channel_id']),
  ]

behavioralparameterslist = Object('behavioralparameterslist', 'Behavioralparameterslist', True)
behavioralparameterslist.id = PQIndex([ORMInt('behav_params_list_id', 'id')], 'behavioralparameterslist_behav_params_list_id_seq')
behavioralparameterslist.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMFloat        ('threshold',                     'threshold'),
    ORMTimestamp    ('version',                       'version'),
  ]

birtreport = Object('birtreport', 'Birtreport', False)
birtreport.id = PQIndex([ORMInt('birt_report_id', 'birt_report_id')], 'birtreport_birt_report_id_seq')
birtreport.fields = [
    ORMInt          ('birt_report_type_id',           'birt_report_type_id'),
    ORMFloat        ('instance_cache_time',           'instance_cache_time'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('version',                       'version'),
  ]

birtreportinstance = Object('birtreportinstance', 'Birtreportinstance', False)
birtreportinstance.id = PQIndex([ORMInt('birt_report_instance_id', 'birt_report_instance_id')], 'birtreportinstance_birt_report_instance_id_seq')
birtreportinstance.fields = [
    ORMInt          ('birt_report_id',                'birt_report_id'),
    ORMTimestamp    ('created_timestamp',             'created_timestamp'),
    ORMString       ('document_file_name',            'document_file_name'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('parameters_hash',               'parameters_hash'),
    ORMFloat        ('state',                         'state'),
  ]

birtreportsession = Object('birtreportsession', 'Birtreportsession', False)
birtreportsession.id = PQIndex([ORMInt('birt_report_session_id', 'birt_report_session_id')], 'birtreportsession_birt_report_session_id_seq')
birtreportsession.fields = [
    ORMInt          ('birt_report_id',                'birt_report_id'),
    ORMInt          ('birt_report_instance_id',       'birt_report_instance_id'),
    ORMTimestamp    ('created_timestamp',             'created_timestamp'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('parameters_hash',               'parameters_hash'),
    ORMString       ('session_id',                    'session_id'),
    ORMFloat        ('state',                         'state'),
    ORMInt          ('user_id',                       'user_id'),
  ]

campaign = Object('campaign', 'Campaign', True)
campaign.id = PQIndex([ORMInt('campaign_id', 'id')], 'campaign_campaign_id_seq')
campaign.fields = [
    ORMInt          ('account_id',                    'account', 'account'),
    ORMInt          ('bill_to_user_id',               'bill_to_user'),
    ORMFloat        ('budget',                        'budget'),
    ORMFloat        ('budget_manual',                 'budget_manual'),
    ORMString       ('campaign_type',                 'campaign_type', default = "'D'"),
    ORMFloat        ('commission',                    'commission', default = "0"),
    ORMFloat        ('daily_budget',                  'daily_budget'),
    ORMTimestamp    ('date_end',                      'date_end'),
    ORMTimestamp    ('date_start',                    'date_start', default = "current_date - 1"),
    ORMString       ('delivery_pacing',               'delivery_pacing', default = "'U'"),
    ORMInt          ('display_status_id',             'display_status_id', default = "1"),
    ORMInt          ('flags',                         'flags'),
    ORMInt          ('freq_cap_id',                   'freq_cap', 'freqcap'),
    ORMTimestamp    ('last_deactivated',              'last_deactivated'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('marketplace',                   'marketplace'),
    ORMFloat        ('max_pub_share',                 'max_pub_share', default = "1"),
    ORMString       ('name',                          'name'),
    ORMInt          ('sales_manager_id',              'sales_manager_id'),
    ORMInt          ('sold_to_user_id',               'sold_to_user'),
    ORMString       ('status',                        'status', default = "'A'"),
    ORMTimestamp    ('version',                       'version'),
  ]

campaignactionstatsdaily = Object('campaignactionstatsdaily', 'Campaignactionstatsdaily', False)
campaignactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
campaignactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

campaignactionstatstotal = Object('campaignactionstatstotal', 'Campaignactionstatstotal', False)
campaignactionstatstotal.id = PQIndex([
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('action_id',                     'action_id'),
  ] )
campaignactionstatstotal.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
  ]

campaignallocation = Object('campaignallocation', 'Campaignallocation', False)
campaignallocation.id = PQIndex([ORMInt('campaign_allocation_id', 'campaign_allocation_id')], 'campaignallocation_campaign_allocation_id_seq')
campaignallocation.fields = [
    ORMFloat        ('amount',                        'amount'),
    ORMInt          ('campaign_id',                   'campaign_id', 'campaign'),
    ORMTimestamp    ('end_date',                      'end_date'),
    ORMInt          ('io_id',                         'io_id', 'insertionorder'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('position',                      'position'),
    ORMTimestamp    ('start_date',                    'start_date'),
    ORMString       ('status',                        'status'),
    ORMFloat        ('utilized_amount',               'utilized_amount'),
    ORMTimestamp    ('version',                       'version'),
  ]

campaigncoloactionstatsdaily = Object('campaigncoloactionstatsdaily', 'Campaigncoloactionstatsdaily', False)
campaigncoloactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
campaigncoloactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

campaigncolotagactionstatsdaily = Object('campaigncolotagactionstatsdaily', 'Campaigncolotagactionstatsdaily', False)
campaigncolotagactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
campaigncolotagactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

campaigncreative = Object('campaigncreative', 'CampaignCreative', True)
campaigncreative.id = PQIndex([ORMInt('cc_id', 'id')], 'campaigncreative_cc_id_seq')
campaigncreative.fields = [
    ORMInt          ('ccg_id',                        'ccg', 'campaigncreativegroup'),
    ORMInt          ('creative_id',                   'creative', 'creative'),
    ORMInt          ('display_status_id',             'display_status_id', default = "1"),
    ORMInt          ('freq_cap_id',                   'freq_cap', 'freqcap'),
    ORMTimestamp    ('last_deactivated',              'last_deactivated'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('set_number',                    'set_number', default = "1"),
    ORMString       ('status',                        'status', default = "'A'"),
    ORMTimestamp    ('version',                       'version'),
    ORMFloat        ('weight',                        'weight'),
  ]

campaigncreativegroup = Object('campaigncreativegroup', 'CampaignCreativeGroup', True)
campaigncreativegroup.id = PQIndex([ORMInt('ccg_id', 'id')], 'campaigncreativegroup_ccg_id_seq')
campaigncreativegroup.fields = [
    ORMFloat        ('budget',                        'budget', default = "1000000"),
    ORMInt          ('campaign_id',                   'campaign', 'campaign'),
    ORMInt          ('ccg_rate_id',                   'ccg_rate', 'ccgrate'),
    ORMString       ('ccg_type',                      'ccg_type', default = "'D'"),
    ORMInt          ('channel_id',                    'channel', 'channel'),
    ORMString       ('channel_target',                'channel_target', default = "'A'"),
    ORMFloat        ('check_interval_num',            'check_interval_num'),
    ORMString       ('check_notes',                   'check_notes'),
    ORMInt          ('check_user_id',                 'check_user_id'),
    ORMString       ('country_code',                  'country_code', 'country', default = "'GN'"),
    ORMInt          ('ctr_reset_id',                  'ctr_reset_id', default = "0"),
    ORMTimestamp    ('cur_date',                      'cur_date'),
    ORMFloat        ('daily_budget',                  'daily_budget'),
    ORMFloat        ('daily_clicks',                  'daily_clicks'),
    ORMFloat        ('daily_imp',                     'daily_imp'),
    ORMTimestamp    ('date_end',                      'date_end'),
    ORMTimestamp    ('date_start',                    'date_start', default = "current_date - 1"),
    ORMString       ('delivery_pacing',               'delivery_pacing', default = "'D'"),
    ORMInt          ('display_status_id',             'display_status_id', default = "1"),
    ORMInt          ('flags',                         'flags', default = "0"),
    ORMInt          ('freq_cap_id',                   'freq_cap', 'freqcap'),
    ORMTimestamp    ('last_check_date',               'last_check_date'),
    ORMTimestamp    ('last_deactivated',              'last_deactivated'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('min_uid_age',                   'min_uid_age', default = "0"),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('next_check_date',               'next_check_date'),
    ORMString       ('optin_status_targeting',        'optin_status_targeting', default = "'YYY'"),
    ORMTimestamp    ('qa_date',                       'qa_date'),
    ORMString       ('qa_description',                'qa_description'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMInt          ('qa_user_id',                    'qa_user_id'),
    ORMFloat        ('realized_budget',               'realized_budget'),
    ORMInt          ('rotation_criteria',             'rotation_criteria'),
    ORMFloat        ('selected_mobile_operators',     'selected_mobile_operators'),
    ORMString       ('status',                        'status', default = "'A'"),
    ORMInt          ('targeting_channel_id',          'targeting_channel_id'),
    ORMString       ('tgt_type',                      'tgt_type', default = "'C'"),
    ORMFloat        ('total_reach',                   'total_reach'),
    ORMInt          ('user_sample_group_end',         'user_sample_group_end'),
    ORMInt          ('user_sample_group_start',       'user_sample_group_start'),
    ORMTimestamp    ('version',                       'version'),
  ]

campaigncredit = Object('campaigncredit', 'Campaigncredit', False)
campaigncredit.id = PQIndex([ORMInt('campaign_credit_id', 'campaign_credit_id')], 'campaigncredit_campaign_credit_id_seq')
campaigncredit.fields = [
    ORMInt          ('account_id',                    'account_id', 'account'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('amount',                        'amount'),
    ORMString       ('description',                   'description'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('pub_payment_option',            'pub_payment_option'),
    ORMString       ('purpose',                       'purpose'),
    ORMTimestamp    ('version',                       'version'),
  ]

campaigncreditallocation = Object('campaigncreditallocation', 'Campaigncreditallocation', False)
campaigncreditallocation.id = PQIndex([ORMInt('camp_credit_alloc_id', 'camp_credit_alloc_id')], 'campaigncreditallocation_camp_credit_alloc_id_seq')
campaigncreditallocation.fields = [
    ORMFloat        ('allocated_amount',              'allocated_amount'),
    ORMInt          ('campaign_credit_id',            'campaign_credit_id', 'campaigncredit'),
    ORMInt          ('campaign_id',                   'campaign_id', 'campaign'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMTimestamp    ('version',                       'version'),
  ]

campaigncreditallocationusage = Object('campaigncreditallocationusage', 'Campaigncreditallocationusage', False)
campaigncreditallocationusage.id = PQIndex([ORMFloat('camp_credit_alloc_id', 'camp_credit_alloc_id')])
campaigncreditallocationusage.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('used_amount',                   'used_amount'),
    ORMTimestamp    ('version',                       'version'),
  ]

campaignexcludedchannel = Object('campaignexcludedchannel', 'Campaignexcludedchannel', False)
campaignexcludedchannel.id = PQIndex([
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('channel_id',                    'channel_id'),
  ] )
campaignexcludedchannel.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

campaignschedule = Object('campaignschedule', 'Campaignschedule', False)
campaignschedule.id = PQIndex([ORMInt('schedule_id', 'schedule_id')], 'campaignschedule_schedule_id_seq')
campaignschedule.fields = [
    ORMInt          ('campaign_id',                   'campaign_id', 'campaign'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('time_from',                     'time_from'),
    ORMFloat        ('time_to',                       'time_to'),
  ]

campaignstatsdaily = Object('campaignstatsdaily', 'Campaignstatsdaily', False)
campaignstatsdaily.id = PQIndex([
    ORMTimestamp    ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('campaign_id',                   'campaign_id'),
  ] )
campaignstatsdaily.fields = [
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('clicks',                        'clicks'),
    ORMFloat        ('credited_actions',              'credited_actions'),
    ORMFloat        ('credited_clicks',               'credited_clicks'),
    ORMFloat        ('credited_imps',                 'credited_imps'),
    ORMInt          ('imps',                          'imps'),
  ]

campaignstatsdailycountry = Object('campaignstatsdailycountry', 'Campaignstatsdailycountry', False)
campaignstatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('campaign_id',                   'campaign_id'),
  ] )
campaignstatsdailycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('requests',                      'requests'),
  ]

campaigntagactionstatsdaily = Object('campaigntagactionstatsdaily', 'Campaigntagactionstatsdaily', False)
campaigntagactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
campaigntagactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

campaignuserstats = Object('campaignuserstats', 'Campaignuserstats', False)
campaignuserstats.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
campaignuserstats.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

campaignuserstatsrunning = DualObject('campaignuserstatsrunning', 'Campaignuserstatsrunning', False)
campaignuserstatsrunning.id = PQIndex([
  ] )
campaignuserstatsrunning.fields = [
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMFloat        ('daily_unique_users',            'daily_unique_users'),
    ORMFloat        ('monthly_unique_users',          'monthly_unique_users'),
    ORMFloat        ('new_unique_users',              'new_unique_users'),
    ORMFloat        ('running_unique_users',          'running_unique_users'),
    ORMFloat        ('weekly_unique_users',           'weekly_unique_users'),
  ]

campaignuserstatstotal = Object('campaignuserstatstotal', 'Campaignuserstatstotal', False)
campaignuserstatstotal.id = PQIndex([ORMInt('campaign_id', 'campaign_id')])
campaignuserstatstotal.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

ccauctionstatsdaily = Object('ccauctionstatsdaily', 'Ccauctionstatsdaily', False)
ccauctionstatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
ccauctionstatsdaily.fields = [
    ORMInt          ('auctions_lost',                 'auctions_lost'),
  ]

ccgaction = Object('ccgaction', 'Ccgaction', True)
ccgaction.id = PQIndex([
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
ccgaction.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgactionstatsdaily = Object('ccgactionstatsdaily', 'Ccgactionstatsdaily', False)
ccgactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
ccgactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

ccgactionstatsdailyadvertiser = Object('ccgactionstatsdailyadvertiser', 'Ccgactionstatsdailyadvertiser', False)
ccgactionstatsdailyadvertiser.id = PQIndex([
    ORMDate         ('advertiser_action_date',        'advertiser_action_date'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
ccgactionstatsdailyadvertiser.fields = [
    ORMInt          ('advertiser_action_month',       'advertiser_action_month'),
    ORMInt          ('advertiser_action_year',        'advertiser_action_year'),
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

ccgactionstatstotal = Object('ccgactionstatstotal', 'Ccgactionstatstotal', False)
ccgactionstatstotal.id = PQIndex([
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('action_id',                     'action_id'),
  ] )
ccgactionstatstotal.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
  ]

ccgaroverride = Object('ccgaroverride', 'Ccgaroverride', False)
ccgaroverride.id = PQIndex([ORMInt('ccg_id', 'ccg_id')])
ccgaroverride.fields = [
    ORMFloat        ('ar',                            'ar'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgauctionstatsdaily = Object('ccgauctionstatsdaily', 'Ccgauctionstatsdaily', False)
ccgauctionstatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
ccgauctionstatsdaily.fields = [
    ORMInt          ('auctions_lost',                 'auctions_lost'),
  ]

ccgauctionstatstotal = Object('ccgauctionstatstotal', 'Ccgauctionstatstotal', False)
ccgauctionstatstotal.id = PQIndex([ORMInt('ccg_id', 'ccg_id')])
ccgauctionstatstotal.fields = [
    ORMInt          ('auctions_lost',                 'auctions_lost'),
    ORMInt          ('selection_failures',            'selection_failures'),
  ]

ccgcoloactionstatsdaily = Object('ccgcoloactionstatsdaily', 'Ccgcoloactionstatsdaily', False)
ccgcoloactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
ccgcoloactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

ccgcoloauctionstatsdaily = Object('ccgcoloauctionstatsdaily', 'Ccgcoloauctionstatsdaily', False)
ccgcoloauctionstatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
ccgcoloauctionstatsdaily.fields = [
    ORMInt          ('auctions_lost',                 'auctions_lost'),
  ]

ccgcolocation = Object('ccgcolocation', 'Ccgcolocation', False)
ccgcolocation.id = PQIndex([
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
ccgcolocation.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgcolotagactionstatsdaily = Object('ccgcolotagactionstatsdaily', 'Ccgcolotagactionstatsdaily', False)
ccgcolotagactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
ccgcolotagactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

ccgcriteriacombination = Object('ccgcriteriacombination', 'Ccgcriteriacombination', False)
ccgcriteriacombination.id = PQIndex([
    ORMInt          ('combination_mask',              'combination_mask'),
    ORMInt          ('ccg_criterion_id',              'ccg_criterion_id'),
  ] )
ccgcriteriacombination.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgcriterion = Object('ccgcriterion', 'Ccgcriterion', False)
ccgcriterion.id = PQIndex([ORMInt('ccg_criterion_id', 'ccg_criterion_id')])
ccgcriterion.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('mask',                          'mask'),
    ORMString       ('name',                          'name'),
  ]

ccgctroverride = Object('ccgctroverride', 'Ccgctroverride', False)
ccgctroverride.id = PQIndex([ORMInt('ccg_id', 'ccg_id')])
ccgctroverride.fields = [
    ORMFloat        ('ctr',                           'ctr'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgdevicechannel = Object('ccgdevicechannel', 'Ccgdevicechannel', False)
ccgdevicechannel.id = PQIndex([
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
  ] )
ccgdevicechannel.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgdevicestatsdaily = Object('ccgdevicestatsdaily', 'Ccgdevicestatsdaily', False)
ccgdevicestatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
  ] )
ccgdevicestatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('pub_amount_adv',                'pub_amount_adv'),
    ORMInt          ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

ccggeochannel = Object('ccggeochannel', 'Ccggeochannel', False)
ccggeochannel.id = PQIndex([
    ORMInt          ('geo_channel_id',                'geo_channel_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
ccggeochannel.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgkeyword = Object('ccgkeyword', 'CCGKeyword', True)
ccgkeyword.id = PQIndex([ORMInt('ccg_keyword_id', 'id')], 'ccgkeyword_ccg_keyword_id_seq')
ccgkeyword.fields = [
    ORMInt          ('ccg_id',                        'ccg', 'campaigncreativegroup'),
    ORMInt          ('channel_id',                    'channel_id', 'channel'),
    ORMString       ('click_url',                     'click_url'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('max_cpc_bid',                   'max_cpc_bid'),
    ORMString       ('original_keyword',              'original_keyword'),
    ORMString       ('status',                        'status'),
    ORMString       ('trigger_type',                  'trigger_type'),
    ORMTimestamp    ('version',                       'version'),
  ]

ccgkeywordctroverride = Object('ccgkeywordctroverride', 'Ccgkeywordctroverride', False)
ccgkeywordctroverride.id = PQIndex([ORMInt('ccg_keyword_id', 'ccg_keyword_id')])
ccgkeywordctroverride.fields = [
    ORMFloat        ('ctr',                           'ctr'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('tow',                           'tow'),
  ]

ccgkeywordstatsdaily = Object('ccgkeywordstatsdaily', 'Ccgkeywordstatsdaily', False)
ccgkeywordstatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_keyword_id',                'ccg_keyword_id'),
    ORMInt          ('cc_id',                         'cc_id'),
  ] )
ccgkeywordstatsdaily.fields = [
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('pub_amount_adv',                'pub_amount_adv'),
  ]

ccgkeywordstatshourly = Object('ccgkeywordstatshourly', 'Ccgkeywordstatshourly', False)
ccgkeywordstatshourly.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_keyword_id',                'ccg_keyword_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('cc_id',                         'cc_id'),
  ] )
ccgkeywordstatshourly.fields = [
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('pub_amount_adv',                'pub_amount_adv'),
  ]

ccgkeywordstatstotal = Object('ccgkeywordstatstotal', 'Ccgkeywordstatstotal', False)
ccgkeywordstatstotal.id = PQIndex([
    ORMInt          ('ccg_keyword_id',                'ccg_keyword_id'),
    ORMInt          ('cc_id',                         'cc_id'),
  ] )
ccgkeywordstatstotal.fields = [
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('pub_amount_adv',                'pub_amount_adv'),
  ]

ccgkeywordstatstow = Object('ccgkeywordstatstow', 'Ccgkeywordstatstow', False)
ccgkeywordstatstow.id = PQIndex([
    ORMInt          ('day_of_week',                   'day_of_week'),
    ORMInt          ('hour',                          'hour'),
    ORMInt          ('ccg_keyword_id',                'ccg_keyword_id'),
    ORMInt          ('cc_id',                         'cc_id'),
  ] )
ccgkeywordstatstow.fields = [
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('pub_amount_adv',                'pub_amount_adv'),
  ]

ccgmobileopchannel = Object('ccgmobileopchannel', 'Ccgmobileopchannel', False)
ccgmobileopchannel.id = PQIndex([
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('mobile_op_channel_id',          'mobile_op_channel_id'),
  ] )
ccgmobileopchannel.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgrate = Object('ccgrate', 'CCGRate', True)
ccgrate.id = PQIndex([ORMInt('ccg_rate_id', 'id')], 'ccgrate_ccg_rate_id_seq')
ccgrate.fields = [
    ORMInt          ('ccg_id',                        'ccg', 'campaigncreativegroup'),
    ORMFloat        ('cpa',                           'cpa'),
    ORMFloat        ('cpc',                           'cpc'),
    ORMFloat        ('cpm',                           'cpm'),
    ORMTimestamp    ('effective_date',                'effective_date'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('rate_type',                     'rate_type'),
  ]

ccgschedule = Object('ccgschedule', 'Ccgschedule', False)
ccgschedule.id = PQIndex([ORMInt('schedule_id', 'schedule_id')], 'ccgschedule_schedule_id_seq')
ccgschedule.fields = [
    ORMInt          ('ccg_id',                        'ccg_id', 'campaigncreativegroup'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('time_from',                     'time_from'),
    ORMFloat        ('time_to',                       'time_to'),
  ]

ccgselectionfailure = Object('ccgselectionfailure', 'Ccgselectionfailure', False)
ccgselectionfailure.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('combination_mask',              'combination_mask'),
  ] )
ccgselectionfailure.fields = [
    ORMInt          ('requests',                      'requests'),
  ]

ccgsite = Object('ccgsite', 'CCGSite', True)
ccgsite.id = PQIndex([
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('site_id',                       'site_id'),
  ] )
ccgsite.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ccgstatsdaily = Object('ccgstatsdaily', 'Ccgstatsdaily', False)
ccgstatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
  ] )
ccgstatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('pub_amount_adv',                'pub_amount_adv'),
    ORMInt          ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

ccgstatsdailycountry = Object('ccgstatsdailycountry', 'Ccgstatsdailycountry', False)
ccgstatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
ccgstatsdailycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_comm_credited_amount', 'adv_camp_comm_credited_amount'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('adv_credited_actions',          'adv_credited_actions'),
    ORMInt          ('adv_credited_clicks',           'adv_credited_clicks'),
    ORMInt          ('adv_credited_imps',             'adv_credited_imps'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('requests',                      'requests'),
  ]

ccgstatstotal = Object('ccgstatstotal', 'Ccgstatstotal', False)
ccgstatstotal.id = PQIndex([ORMInt('ccg_id', 'ccg_id')])
ccgstatstotal.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_camp_credited_amount',      'adv_camp_credited_amount'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('pub_amount_adv',                'pub_amount_adv'),
    ORMFloat        ('pub_comm_amount_adv',           'pub_comm_amount_adv'),
  ]

ccgtagactionstatsdaily = Object('ccgtagactionstatsdaily', 'Ccgtagactionstatsdaily', False)
ccgtagactionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('action_id',                     'action_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
ccgtagactionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

ccguserstats = Object('ccguserstats', 'Ccguserstats', False)
ccguserstats.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
ccguserstats.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

ccguserstatsrunning = DualObject('ccguserstatsrunning', 'Ccguserstatsrunning', False)
ccguserstatsrunning.id = PQIndex([
  ] )
ccguserstatsrunning.fields = [
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMFloat        ('daily_unique_users',            'daily_unique_users'),
    ORMFloat        ('monthly_unique_users',          'monthly_unique_users'),
    ORMFloat        ('new_unique_users',              'new_unique_users'),
    ORMFloat        ('running_unique_users',          'running_unique_users'),
    ORMFloat        ('weekly_unique_users',           'weekly_unique_users'),
  ]

ccguserstatstotal = Object('ccguserstatstotal', 'Ccguserstatstotal', False)
ccguserstatstotal.id = PQIndex([ORMInt('ccg_id', 'ccg_id')])
ccguserstatstotal.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

ccuserstats = Object('ccuserstats', 'Ccuserstats', False)
ccuserstats.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
ccuserstats.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

ccuserstatsrunning = DualObject('ccuserstatsrunning', 'Ccuserstatsrunning', False)
ccuserstatsrunning.id = PQIndex([
  ] )
ccuserstatsrunning.fields = [
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMFloat        ('daily_unique_users',            'daily_unique_users'),
    ORMFloat        ('monthly_unique_users',          'monthly_unique_users'),
    ORMFloat        ('new_unique_users',              'new_unique_users'),
    ORMFloat        ('running_unique_users',          'running_unique_users'),
    ORMFloat        ('weekly_unique_users',           'weekly_unique_users'),
  ]

ccuserstatstotal = Object('ccuserstatstotal', 'Ccuserstatstotal', False)
ccuserstatstotal.id = PQIndex([ORMInt('cc_id', 'cc_id')])
ccuserstatstotal.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

change_password_uid = Object('change_password_uid', 'Change_password_uid', False)
change_password_uid.id = PQIndex([ORMInt('change_password_id', 'change_password_id')], 'change_password_uid_change_password_id_seq')
change_password_uid.fields = [
    ORMString       ('changing_uid',                  'changing_uid'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('user_credential_id',            'user_credential_id'),
    ORMTimestamp    ('version',                       'version'),
  ]

channel = Object('channel', 'Channel', True)
channel.id = PQIndex([ORMInt('channel_id', 'id')], 'channel_channel_id_seq')
channel.fields = [
    ORMInt          ('account_id',                    'account', 'account'),
    ORMString       ('address',                       'address'),
    ORMString       ('base_keyword',                  'base_keyword'),
    ORMInt          ('behav_params_list_id',          'behav_params_list_id', 'behavioralparameterslist'),
    ORMInt          ('channel_list_id',               'channel_list_id'),
    ORMString       ('channel_name_macro',            'channel_name_macro'),
    ORMInt          ('channel_rate_id',               'channel_rate_id', 'channelrate'),
    ORMString       ('channel_type',                  'type'),
    ORMFloat        ('check_interval_num',            'check_interval_num'),
    ORMString       ('check_notes',                   'check_notes'),
    ORMInt          ('check_user_id',                 'check_user_id'),
    ORMString       ('city_list',                     'city_list'),
    ORMString       ('country_code',                  'country_code', 'country', default = "'GN'"),
    ORMTimestamp    ('created_date',                  'created_date'),
    ORMString       ('description',                   'description'),
    ORMString       ('discover_annotation',           'discover_annotation'),
    ORMString       ('discover_query',                'discover_query'),
    ORMInt          ('display_status_id',             'display_status_id', default = "1"),
    ORMFloat        ('distinct_url_triggers_count',   'distinct_url_triggers_count'),
    ORMString       ('expression',                    'expression'),
    ORMFloat        ('flags',                         'flags'),
    ORMInt          ('freq_cap_id',                   'freq_cap_id', 'freqcap'),
    ORMString       ('geo_type',                      'geo_type'),
    ORMString       ('keyword_trigger_macro',         'keyword_trigger_macro'),
    ORMString       ('language',                      'language'),
    ORMTimestamp    ('last_check_date',               'last_check_date'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('latitude',                      'latitude'),
    ORMFloat        ('longitude',                     'longitude'),
    ORMFloat        ('message_sent',                  'message_sent', default = "0"),
    ORMString       ('name',                          'name'),
    ORMString       ('namespace',                     'channel_namespace'),
    ORMString       ('newsgate_category_name',        'newsgate_category_name'),
    ORMTimestamp    ('next_check_date',               'next_check_date'),
    ORMInt          ('parent_channel_id',             'parent_channel_id'),
    ORMTimestamp    ('qa_date',                       'qa_date'),
    ORMString       ('qa_description',                'qa_description'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMInt          ('qa_user_id',                    'qa_user_id'),
    ORMInt          ('radius',                        'radius'),
    ORMString       ('radius_units',                  'radius_units'),
    ORMInt          ('size_id',                       'size_id'),
    ORMString       ('status',                        'status'),
    ORMTimestamp    ('status_change_date',            'status_change_date'),
    ORMInt          ('superseded_by_channel_id',      'superseded_by_channel_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
    ORMString       ('triggers_status',               'triggers_status'),
    ORMTimestamp    ('triggers_version',              'triggers_version'),
    ORMTimestamp    ('version',                       'version'),
    ORMString       ('visibility',                    'visibility', default = "'PUB'"),
  ]
channel.asks = [Select('name', ['name'])]

channelcategory = Object('channelcategory', 'Channelcategory', False)
channelcategory.id = PQIndex([
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('category_channel_id',           'category_channel_id'),
  ] )
channelcategory.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

channelcountstats = Object('channelcountstats', 'Channelcountstats', False)
channelcountstats.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMInt          ('channel_count',                 'channel_count'),
  ] )
channelcountstats.fields = [
    ORMInt          ('users_count',                   'users_count'),
  ]

channelimpinventory = Object('channelimpinventory', 'Channelimpinventory', False)
channelimpinventory.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('ccg_type',                      'ccg_type'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
channelimpinventory.fields = [
    ORMFloat        ('actions',                       'actions'),
    ORMFloat        ('clicks',                        'clicks'),
    ORMFloat        ('impops_no_imp',                 'impops_no_imp'),
    ORMFloat        ('impops_no_imp_user_count',      'impops_no_imp_user_count'),
    ORMFloat        ('impops_no_imp_value',           'impops_no_imp_value'),
    ORMFloat        ('impops_user_count',             'impops_user_count'),
    ORMFloat        ('imps',                          'imps'),
    ORMFloat        ('imps_other',                    'imps_other'),
    ORMFloat        ('imps_other_user_count',         'imps_other_user_count'),
    ORMFloat        ('imps_other_value',              'imps_other_value'),
    ORMFloat        ('imps_user_count',               'imps_user_count'),
    ORMFloat        ('imps_value',                    'imps_value'),
    ORMFloat        ('revenue',                       'revenue'),
  ]

channelinventory = Object('channelinventory', 'ChannelInventory', True)
channelinventory.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
channelinventory.fields = [
    ORMFloat        ('active_user_count',             'active_user_count'),
    ORMFloat        ('hits',                          'hits'),
    ORMFloat        ('hits_kws',                      'hits_kws'),
    ORMFloat        ('hits_search_kws',               'hits_search_kws'),
    ORMFloat        ('hits_url_kws',                  'hits_url_kws'),
    ORMFloat        ('hits_urls',                     'hits_urls'),
    ORMFloat        ('sum_ecpm',                      'sum_ecpm'),
    ORMFloat        ('total_user_count',              'total_user_count'),
  ]

channelinventorybycpm = Object('channelinventorybycpm', 'Channelinventorybycpm', False)
channelinventorybycpm.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('creative_size_id',              'creative_size_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMFloat        ('ecpm',                          'ecpm'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('channel_country_code',          'channel_country_code'),
  ] )
channelinventorybycpm.fields = [
    ORMInt          ('impops',                        'impops'),
    ORMFloat        ('user_count',                    'user_count'),
  ]

channelinventorybycpmmonthly = Object('channelinventorybycpmmonthly', 'Channelinventorybycpmmonthly', False)
channelinventorybycpmmonthly.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('creative_size_id',              'creative_size_id'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMFloat        ('ecpm',                          'ecpm'),
    ORMString       ('channel_country_code',          'channel_country_code'),
  ] )
channelinventorybycpmmonthly.fields = [
    ORMInt          ('impops',                        'impops'),
    ORMFloat        ('user_count',                    'user_count'),
  ]

channelinventorybycpmmonthlydates = Object('channelinventorybycpmmonthlydates', 'Channelinventorybycpmmonthlydates', False)
channelinventorybycpmmonthlydates.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('creative_size_id',              'creative_size_id'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_country_code',          'channel_country_code'),
  ] )
channelinventorybycpmmonthlydates.fields = [
    ORMString       ('dates',                         'dates'),
    ORMFloat        ('impops',                        'impops'),
    ORMFloat        ('user_count',                    'user_count'),
  ]

channelinventoryestimstats = Object('channelinventoryestimstats', 'Channelinventoryestimstats', False)
channelinventoryestimstats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMFloat        ('match_level',                   'match_level'),
  ] )
channelinventoryestimstats.fields = [
    ORMFloat        ('users_from_now',                'users_from_now'),
    ORMFloat        ('users_regular',                 'users_regular'),
  ]

channeloverlapuserstats = Object('channeloverlapuserstats', 'Channeloverlapuserstats', False)
channeloverlapuserstats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('channel1_id',                   'channel1_id'),
    ORMInt          ('channel2_id',                   'channel2_id'),
  ] )
channeloverlapuserstats.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

channelrate = Object('channelrate', 'Channelrate', True)
channelrate.id = PQIndex([ORMInt('channel_rate_id', 'channel_rate_id')], 'channelrate_channel_rate_id_seq')
channelrate.fields = [
    ORMInt          ('channel_id',                    'channel_id', 'channel'),
    ORMFloat        ('cpc',                           'cpc'),
    ORMFloat        ('cpm',                           'cpm'),
    ORMInt          ('currency_id',                   'currency_id', 'currency'),
    ORMTimestamp    ('effective_date',                'effective_date'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('rate_type',                     'rate_type'),
  ]

channelstats = Object('channelstats', 'Channelstats', False)
channelstats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
channelstats.fields = [
    ORMFloat        ('actions_d',                     'actions_d'),
    ORMFloat        ('actions_t',                     'actions_t'),
    ORMFloat        ('active_user_count',             'active_user_count'),
    ORMFloat        ('clicks_d',                      'clicks_d'),
    ORMFloat        ('clicks_t',                      'clicks_t'),
    ORMFloat        ('hits',                          'hits'),
    ORMFloat        ('hits_kws',                      'hits_kws'),
    ORMFloat        ('hits_search_kws',               'hits_search_kws'),
    ORMFloat        ('hits_url_kws',                  'hits_url_kws'),
    ORMFloat        ('hits_urls',                     'hits_urls'),
    ORMFloat        ('impops_no_imp_d',               'impops_no_imp_d'),
    ORMFloat        ('impops_no_imp_t',               'impops_no_imp_t'),
    ORMFloat        ('impops_no_imp_user_count_d',    'impops_no_imp_user_count_d'),
    ORMFloat        ('impops_no_imp_user_count_t',    'impops_no_imp_user_count_t'),
    ORMFloat        ('impops_no_imp_value_d',         'impops_no_imp_value_d'),
    ORMFloat        ('impops_no_imp_value_t',         'impops_no_imp_value_t'),
    ORMFloat        ('impops_user_count_d',           'impops_user_count_d'),
    ORMFloat        ('impops_user_count_t',           'impops_user_count_t'),
    ORMFloat        ('imps_d',                        'imps_d'),
    ORMFloat        ('imps_other_d',                  'imps_other_d'),
    ORMFloat        ('imps_other_t',                  'imps_other_t'),
    ORMFloat        ('imps_other_user_count_d',       'imps_other_user_count_d'),
    ORMFloat        ('imps_other_user_count_t',       'imps_other_user_count_t'),
    ORMFloat        ('imps_other_value_d',            'imps_other_value_d'),
    ORMFloat        ('imps_other_value_t',            'imps_other_value_t'),
    ORMFloat        ('imps_t',                        'imps_t'),
    ORMFloat        ('imps_user_count_d',             'imps_user_count_d'),
    ORMFloat        ('imps_user_count_t',             'imps_user_count_t'),
    ORMFloat        ('imps_value_d',                  'imps_value_d'),
    ORMFloat        ('imps_value_t',                  'imps_value_t'),
    ORMFloat        ('revenue_d',                     'revenue_d'),
    ORMFloat        ('revenue_t',                     'revenue_t'),
    ORMInt          ('smonth',                        'smonth'),
    ORMFloat        ('sum_ecpm',                      'sum_ecpm'),
    ORMInt          ('syear',                         'syear'),
    ORMFloat        ('total_user_count',              'total_user_count'),
  ]

channeltrigger = Object('channeltrigger', 'Channeltrigger', True)
channeltrigger.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')], 'channeltrigger_channel_trigger_id_seq')
channeltrigger.fields = [
    ORMInt          ('channel_id',                    'channel_id', 'channel'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative', default = "'N'"),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id', 'triggers'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltriggerstats = Object('channeltriggerstats', 'Channeltriggerstats', False)
channeltriggerstats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
    ORMInt          ('channel_trigger_id',            'channel_trigger_id'),
  ] )
channeltriggerstats.fields = [
    ORMFloat        ('approximated_clicks',           'approximated_clicks'),
    ORMFloat        ('approximated_imps',             'approximated_imps'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('hits',                          'hits'),
  ]

channeltriggerstatstotal = Object('channeltriggerstatstotal', 'Channeltriggerstatstotal', False)
channeltriggerstatstotal.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltriggerstatstotal.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMDate         ('last_hit_date',                 'last_hit_date'),
  ]

channeltrigger_pcc_aa_channel_type_a = Object('channeltrigger_pcc_aa_channel_type_a', 'Channeltrigger_pcc_aa_channel_type_a', False)
channeltrigger_pcc_aa_channel_type_a.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_aa_channel_type_a.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_aa_channel_type_d = Object('channeltrigger_pcc_aa_channel_type_d', 'Channeltrigger_pcc_aa_channel_type_d', False)
channeltrigger_pcc_aa_channel_type_d.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_aa_channel_type_d.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_aa_channel_type_s = Object('channeltrigger_pcc_aa_channel_type_s', 'Channeltrigger_pcc_aa_channel_type_s', False)
channeltrigger_pcc_aa_channel_type_s.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_aa_channel_type_s.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_cn_channel_type_a = Object('channeltrigger_pcc_cn_channel_type_a', 'Channeltrigger_pcc_cn_channel_type_a', False)
channeltrigger_pcc_cn_channel_type_a.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_cn_channel_type_a.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_cn_channel_type_d = Object('channeltrigger_pcc_cn_channel_type_d', 'Channeltrigger_pcc_cn_channel_type_d', False)
channeltrigger_pcc_cn_channel_type_d.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_cn_channel_type_d.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_cn_channel_type_s = Object('channeltrigger_pcc_cn_channel_type_s', 'Channeltrigger_pcc_cn_channel_type_s', False)
channeltrigger_pcc_cn_channel_type_s.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_cn_channel_type_s.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_ru_channel_type_a = Object('channeltrigger_pcc_ru_channel_type_a', 'Channeltrigger_pcc_ru_channel_type_a', False)
channeltrigger_pcc_ru_channel_type_a.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_ru_channel_type_a.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_ru_channel_type_d = Object('channeltrigger_pcc_ru_channel_type_d', 'Channeltrigger_pcc_ru_channel_type_d', False)
channeltrigger_pcc_ru_channel_type_d.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_ru_channel_type_d.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channeltrigger_pcc_ru_channel_type_s = Object('channeltrigger_pcc_ru_channel_type_s', 'Channeltrigger_pcc_ru_channel_type_s', False)
channeltrigger_pcc_ru_channel_type_s.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
channeltrigger_pcc_ru_channel_type_s.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('masked',                        'masked'),
    ORMBool         ('negative',                      'negative'),
    ORMString       ('original_trigger',              'original_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_group',                 'trigger_group'),
    ORMInt          ('trigger_id',                    'trigger_id'),
    ORMString       ('trigger_type',                  'trigger_type'),
  ]

channelusagestatshourly = Object('channelusagestatshourly', 'Channelusagestatshourly', False)
channelusagestatshourly.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
channelusagestatshourly.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('revenue',                       'revenue'),
  ]

channelusagestatstotal = Object('channelusagestatstotal', 'Channelusagestatstotal', False)
channelusagestatstotal.id = PQIndex([ORMInt('channel_id', 'channel_id')])
channelusagestatstotal.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMDate         ('last_use',                      'last_use'),
    ORMFloat        ('revenue',                       'revenue'),
  ]

clobparams = Object('clobparams', 'Clobparams', False)
clobparams.id = PQIndex([
    ORMString       ('name',                          'name'),
    ORMInt          ('account_id',                    'account_id'),
  ] )
clobparams.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('value',                         'value'),
    ORMTimestamp    ('version',                       'version'),
  ]

cmprequeststatshourly = Object('cmprequeststatshourly', 'Cmprequeststatshourly', False)
cmprequeststatshourly.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('channel_rate_id',               'channel_rate_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMInt          ('size_id',                       'size_id'),
  ] )
cmprequeststatshourly.fields = [
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount_cmp',                'adv_amount_cmp'),
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('cmp_account_id',                'cmp_account_id'),
    ORMFloat        ('cmp_amount',                    'cmp_amount'),
    ORMFloat        ('cmp_amount_global',             'cmp_amount_global'),
    ORMDate         ('cmp_sdate',                     'cmp_sdate'),
    ORMInt          ('imps',                          'imps'),
  ]

cmpstatsdaily = Object('cmpstatsdaily', 'Cmpstatsdaily', False)
cmpstatsdaily.id = PQIndex([
    ORMDate         ('cmp_sdate',                     'cmp_sdate'),
    ORMInt          ('cmp_account_id',                'cmp_account_id'),
    ORMInt          ('channel_id',                    'channel_id'),
  ] )
cmpstatsdaily.fields = [
    ORMInt          ('clicks',                        'clicks'),
    ORMFloat        ('cmp_amount',                    'cmp_amount'),
    ORMInt          ('imps',                          'imps'),
  ]

colocation = Object('colocation', 'Colocation', True)
colocation.id = PQIndex([ORMInt('colo_id', 'id')], 'colocation_colo_id_seq')
colocation.fields = [
    ORMInt          ('account_id',                    'account', 'account'),
    ORMInt          ('colo_rate_id',                  'colo_rate', 'colocationrate'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('optout_serving',                'optout_serving', default = "'NON_OPTOUT'"),
    ORMString       ('status',                        'status'),
    ORMTimestamp    ('version',                       'version'),
  ]
colocation.asks = [Select('name', ['name'])]

colocationrate = Object('colocationrate', 'ColocationRate', True)
colocationrate.id = PQIndex([ORMInt('colo_rate_id', 'id')], 'colocationrate_colo_rate_id_seq')
colocationrate.fields = [
    ORMInt          ('colo_id',                       'colo', 'colocation'),
    ORMTimestamp    ('effective_date',                'effective_date', default = "now()"),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('revenue_share',                 'revenue_share'),
  ]

colorequeststats = Object('colorequeststats', 'Colorequeststats', False)
colorequeststats.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('user_status',                   'user_status'),
  ] )
colorequeststats.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('imps_unverified',               'imps_unverified'),
    ORMInt          ('profiling_requests',            'profiling_requests'),
    ORMInt          ('requests',                      'requests'),
  ]

colostats = Object('colostats', 'Colostats', True)
colostats.id = PQIndex([ORMInt('colo_id', 'colo_id')])
colostats.fields = [
    ORMDate         ('last_campaign_update',          'last_campaign_update'),
    ORMDate         ('last_channel_update',           'last_channel_update'),
    ORMDate         ('last_stats_upload',             'last_stats_upload'),
    ORMString       ('software_version',              'software_version'),
  ]

colouserstats = Object('colouserstats', 'Colouserstats', False)
colouserstats.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
colouserstats.fields = [
    ORMInt          ('network_unique_users',          'network_unique_users'),
    ORMInt          ('profiling_unique_users',        'profiling_unique_users'),
    ORMInt          ('unique_hids',                   'unique_hids'),
    ORMInt          ('unique_users',                  'unique_users'),
  ]

colouserstatsrunning = DualObject('colouserstatsrunning', 'Colouserstatsrunning', False)
colouserstatsrunning.id = PQIndex([
  ] )
colouserstatsrunning.fields = [
    ORMFloat        ('calendar_monthly_network_unique_users','calendar_monthly_network_unique_users'),
    ORMFloat        ('calendar_monthly_profiling_unique_users','calendar_monthly_profiling_unique_users'),
    ORMFloat        ('calendar_monthly_unique_hids',  'calendar_monthly_unique_hids'),
    ORMFloat        ('calendar_monthly_unique_users', 'calendar_monthly_unique_users'),
    ORMFloat        ('calendar_weekly_network_unique_users','calendar_weekly_network_unique_users'),
    ORMFloat        ('calendar_weekly_profiling_unique_users','calendar_weekly_profiling_unique_users'),
    ORMFloat        ('calendar_weekly_unique_hids',   'calendar_weekly_unique_hids'),
    ORMFloat        ('calendar_weekly_unique_users',  'calendar_weekly_unique_users'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMFloat        ('daily_network_unique_users',    'daily_network_unique_users'),
    ORMFloat        ('daily_profiling_unique_users',  'daily_profiling_unique_users'),
    ORMFloat        ('daily_unique_hids',             'daily_unique_hids'),
    ORMFloat        ('daily_unique_users',            'daily_unique_users'),
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMFloat        ('monthly_network_unique_users',  'monthly_network_unique_users'),
    ORMFloat        ('monthly_profiling_unique_users','monthly_profiling_unique_users'),
    ORMFloat        ('monthly_unique_hids',           'monthly_unique_hids'),
    ORMFloat        ('monthly_unique_users',          'monthly_unique_users'),
    ORMFloat        ('new_network_unique_users',      'new_network_unique_users'),
    ORMFloat        ('new_profiling_unique_users',    'new_profiling_unique_users'),
    ORMFloat        ('new_unique_hids',               'new_unique_hids'),
    ORMFloat        ('new_unique_users',              'new_unique_users'),
    ORMFloat        ('running_network_unique_users',  'running_network_unique_users'),
    ORMFloat        ('running_profiling_unique_users','running_profiling_unique_users'),
    ORMFloat        ('running_unique_hids',           'running_unique_hids'),
    ORMFloat        ('running_unique_users',          'running_unique_users'),
    ORMFloat        ('shifted_monthly_network_unique_users','shifted_monthly_network_unique_users'),
    ORMFloat        ('shifted_monthly_profiling_unique_users','shifted_monthly_profiling_unique_users'),
    ORMFloat        ('shifted_monthly_unique_hids',   'shifted_monthly_unique_hids'),
    ORMFloat        ('shifted_monthly_unique_users',  'shifted_monthly_unique_users'),
    ORMFloat        ('shifted_weekly_network_unique_users','shifted_weekly_network_unique_users'),
    ORMFloat        ('shifted_weekly_profiling_unique_users','shifted_weekly_profiling_unique_users'),
    ORMFloat        ('shifted_weekly_unique_hids',    'shifted_weekly_unique_hids'),
    ORMFloat        ('shifted_weekly_unique_users',   'shifted_weekly_unique_users'),
    ORMFloat        ('weekly_network_unique_users',   'weekly_network_unique_users'),
    ORMFloat        ('weekly_profiling_unique_users', 'weekly_profiling_unique_users'),
    ORMFloat        ('weekly_unique_hids',            'weekly_unique_hids'),
    ORMFloat        ('weekly_unique_users',           'weekly_unique_users'),
  ]

colouserstatstotal = Object('colouserstatstotal', 'Colouserstatstotal', False)
colouserstatstotal.id = PQIndex([ORMInt('colo_id', 'colo_id')])
colouserstatstotal.fields = [
    ORMFloat        ('network_unique_users',          'network_unique_users'),
    ORMFloat        ('profiling_unique_users',        'profiling_unique_users'),
    ORMFloat        ('unique_hids',                   'unique_hids'),
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

contentcategory = Object('contentcategory', 'Contentcategory', False)
contentcategory.id = PQIndex([ORMInt('content_category_id', 'content_category_id')], 'contentcategory_content_category_id_seq')
contentcategory.fields = [
    ORMString       ('country_code',                  'country_code', 'country'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('version',                       'version'),
  ]

conversioncategory = Object('conversioncategory', 'Conversioncategory', False)
conversioncategory.id = PQIndex([ORMInt('conv_category_id', 'conv_category_id')])
conversioncategory.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
  ]

conversionstatsdaily = Object('conversionstatsdaily', 'Conversionstatsdaily', False)
conversionstatsdaily.id = PQIndex([
    ORMDate         ('country_action_date',           'country_action_date'),
    ORMInt          ('action_id',                     'action_id'),
    ORMDate         ('action_date',                   'action_date'),
  ] )
conversionstatsdaily.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
    ORMInt          ('country_action_month',          'country_action_month'),
    ORMInt          ('country_action_year',           'country_action_year'),
    ORMInt          ('min_days_since_click',          'min_days_since_click'),
    ORMInt          ('min_days_since_imp',            'min_days_since_imp'),
  ]

conversionstatstotal = Object('conversionstatstotal', 'Conversionstatstotal', False)
conversionstatstotal.id = PQIndex([ORMInt('action_id', 'action_id')])
conversionstatstotal.fields = [
    ORMInt          ('conversions',                   'conversions'),
    ORMInt          ('conversions_post_click',        'conversions_post_click'),
    ORMInt          ('conversions_post_imp',          'conversions_post_imp'),
  ]

country = Object('country', 'Country', True)
country.id = PQIndex([ORMString('country_code', 'country_code')], 'country_country_id_seq')
country.fields = [
    ORMString       ('ad_footer_url',                 'ad_footer_url'),
    ORMString       ('ad_tag_domain',                 'ad_tag_domain'),
    ORMString       ('adserving_domain',              'adserving_domain'),
    ORMString       ('conversion_tag_domain',         'conversion_tag_domain'),
    ORMInt          ('country_id',                    'country_id'),
    ORMInt          ('currency_id',                   'currency_id', 'currency'),
    ORMFloat        ('default_agency_commission',     'default_agency_commission'),
    ORMFloat        ('default_payment_terms',         'default_payment_terms'),
    ORMFloat        ('default_vat_rate',              'default_vat_rate'),
    ORMString       ('discover_domain',               'discover_domain'),
    ORMFloat        ('high_channel_threshold',        'high_channel_threshold'),
    ORMInt          ('invoice_custom_report_id',      'invoice_custom_report_id'),
    ORMString       ('language',                      'language'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('low_channel_threshold',         'low_channel_threshold'),
    ORMFloat        ('max_url_trigger_share',         'max_url_trigger_share'),
    ORMFloat        ('min_tag_visibility',            'min_tag_visibility'),
    ORMFloat        ('min_url_trigger_threshold',     'min_url_trigger_threshold'),
    ORMFloat        ('sortorder',                     'sortorder'),
    ORMString       ('static_domain',                 'static_domain'),
    ORMInt          ('timezone_id',                   'timezone_id'),
    ORMBool         ('vat_enabled',                   'vat_enabled'),
    ORMBool         ('vat_number_input_enabled',      'vat_number_input_enabled'),
    ORMTimestamp    ('version',                       'version'),
  ]

countryaddressfield = Object('countryaddressfield', 'Countryaddressfield', False)
countryaddressfield.id = PQIndex([ORMInt('country_address_field_id', 'country_address_field_id')], 'countryaddressfield_country_address_field_id_seq')
countryaddressfield.fields = [
    ORMString       ('country_code',                  'country_code', 'country'),
    ORMString       ('field_name',                    'field_name'),
    ORMInt          ('flags',                         'flags'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMFloat        ('order_number',                  'order_number'),
    ORMString       ('resource_key',                  'resource_key'),
  ]

createduserstats = Object('createduserstats', 'Createduserstats', False)
createduserstats.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('create_date',                   'create_date'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
createduserstats.fields = [
    ORMInt          ('network_unique_users',          'network_unique_users'),
    ORMInt          ('profiling_unique_users',        'profiling_unique_users'),
    ORMInt          ('unique_hids',                   'unique_hids'),
    ORMInt          ('unique_users',                  'unique_users'),
  ]

creative = Object('creative', 'Creative', True)
creative.id = PQIndex([ORMInt('creative_id', 'id')], 'creative_creative_id_seq')
creative.fields = [
    ORMInt          ('account_id',                    'account', 'account'),
    ORMInt          ('display_status_id',             'display_status_id', default = "1"),
    ORMBool         ('expandable',                    'expandable', default = "'N'"),
    ORMString       ('expansion',                     'expansion'),
    ORMInt          ('flags',                         'flags'),
    ORMTimestamp    ('last_deactivated',              'last_deactivated'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('qa_date',                       'qa_date'),
    ORMString       ('qa_description',                'qa_description'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMInt          ('qa_user_id',                    'qa_user_id'),
    ORMInt          ('size_id',                       'size', 'creativesize'),
    ORMString       ('status',                        'status'),
    ORMInt          ('template_id',                   'template_id', 'template'),
    ORMTimestamp    ('version',                       'version'),
  ]

creativecategory = Object('creativecategory', 'CreativeCategory', True)
creativecategory.id = PQIndex([ORMInt('creative_category_id', 'id')], 'creativecategory_creative_category_id_seq')
creativecategory.fields = [
    ORMInt          ('cct_id',                        'type', 'creativecategorytype'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('qa_status',                     'qa_status', default = "'A'"),
    ORMTimestamp    ('version',                       'version'),
  ]

creativecategorytype = Object('creativecategorytype', 'Creativecategorytype', True)
creativecategorytype.id = PQIndex([ORMInt('cct_id', 'cct_id')])
creativecategorytype.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('version',                       'version'),
  ]

creativecategory_account = Object('creativecategory_account', 'Creativecategory_account', False)
creativecategory_account.id = PQIndex([
    ORMFloat        ('creative_category_id',          'creative_category_id'),
    ORMFloat        ('account_id',                    'account_id'),
  ] )
creativecategory_account.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

creativecategory_creative = Object('creativecategory_creative', 'CreativeCategory_Creative', True)
creativecategory_creative.id = PQIndex([
    ORMInt          ('creative_category_id',          'creative_category_id'),
    ORMInt          ('creative_id',                   'creative_id'),
  ] )
creativecategory_creative.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

creativecategory_template = Object('creativecategory_template', 'Creativecategory_template', False)
creativecategory_template.id = PQIndex([
    ORMInt          ('creative_category_id',          'creative_category_id'),
    ORMInt          ('template_id',                   'template_id'),
  ] )
creativecategory_template.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

creativeoptgroupstate = Object('creativeoptgroupstate', 'Creativeoptgroupstate', False)
creativeoptgroupstate.id = PQIndex([
    ORMInt          ('option_group_id',               'option_group_id'),
    ORMInt          ('creative_id',                   'creative_id'),
  ] )
creativeoptgroupstate.fields = [
    ORMBool         ('collapsed',                     'collapsed'),
    ORMBool         ('enabled',                       'enabled'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMTimestamp    ('version',                       'version'),
  ]

creativeoptionvalue = Object('creativeoptionvalue', 'CreativeOptionValue', True)
creativeoptionvalue.id = PQIndex([
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('option_id',                     'option_id'),
  ] )
creativeoptionvalue.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('value',                         'value'),
    ORMTimestamp    ('version',                       'version'),
  ]

creativerejectreason = Object('creativerejectreason', 'Creativerejectreason', False)
creativerejectreason.id = PQIndex([ORMInt('reject_reason_id', 'reject_reason_id')], 'creativerejectreason_reject_reason_id_seq')
creativerejectreason.fields = [
    ORMString       ('description',                   'description'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

creativesize = Object('creativesize', 'Creativesize', True)
creativesize.id = PQIndex([ORMInt('size_id', 'size_id')], 'creativesize_size_id_seq')
creativesize.fields = [
    ORMInt          ('flags',                         'flags'),
    ORMFloat        ('height',                        'height'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('max_height',                    'max_height'),
    ORMFloat        ('max_width',                     'max_width'),
    ORMString       ('name',                          'name'),
    ORMString       ('protocol_name',                 'protocol_name'),
    ORMInt          ('size_type_id',                  'size_type_id'),
    ORMString       ('status',                        'status'),
    ORMTimestamp    ('version',                       'version'),
    ORMFloat        ('width',                         'width'),
  ]

creativesizeexpansion = Object('creativesizeexpansion', 'Creativesizeexpansion', False)
creativesizeexpansion.id = PQIndex([
    ORMInt          ('size_id',                       'size_id'),
    ORMString       ('expansion',                     'expansion'),
  ] )
creativesizeexpansion.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

creative_tagsize = Object('creative_tagsize', 'Creative_tagsize', True)
creative_tagsize.id = PQIndex([
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('size_id',                       'size_id'),
  ] )
creative_tagsize.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

creative_tagsizetype = Object('creative_tagsizetype', 'Creative_tagsizetype', False)
creative_tagsizetype.id = PQIndex([
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('size_type_id',                  'size_type_id'),
  ] )
creative_tagsizetype.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ctralgadvertiserexclusion = Object('ctralgadvertiserexclusion', 'Ctralgadvertiserexclusion', False)
ctralgadvertiserexclusion.id = PQIndex([
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
  ] )
ctralgadvertiserexclusion.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ctralgcampaignexclusion = Object('ctralgcampaignexclusion', 'Ctralgcampaignexclusion', False)
ctralgcampaignexclusion.id = PQIndex([
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('campaign_id',                   'campaign_id'),
  ] )
ctralgcampaignexclusion.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

ctralgorithm = Object('ctralgorithm', 'Ctralgorithm', False)
ctralgorithm.id = PQIndex([ORMString('country_code', 'country_code')])
ctralgorithm.fields = [
    ORMFloat        ('campaign_tow_level',            'campaign_tow_level'),
    ORMFloat        ('ccgkeyword_kw_ctr_level',       'ccgkeyword_kw_ctr_level'),
    ORMFloat        ('ccgkeyword_kw_tow_level',       'ccgkeyword_kw_tow_level'),
    ORMFloat        ('ccgkeyword_tg_ctr_level',       'ccgkeyword_tg_ctr_level'),
    ORMFloat        ('ccgkeyword_tg_tow_level',       'ccgkeyword_tg_tow_level'),
    ORMFloat        ('clicks_interval1_days',         'clicks_interval1_days'),
    ORMFloat        ('clicks_interval1_weight',       'clicks_interval1_weight'),
    ORMFloat        ('clicks_interval2_days',         'clicks_interval2_days'),
    ORMFloat        ('clicks_interval2_weight',       'clicks_interval2_weight'),
    ORMFloat        ('clicks_interval3_weight',       'clicks_interval3_weight'),
    ORMFloat        ('cpa_random_imps',               'cpa_random_imps'),
    ORMFloat        ('cpc_random_imps',               'cpc_random_imps'),
    ORMFloat        ('imps_interval1_days',           'imps_interval1_days'),
    ORMFloat        ('imps_interval1_weight',         'imps_interval1_weight'),
    ORMFloat        ('imps_interval2_days',           'imps_interval2_days'),
    ORMFloat        ('imps_interval2_weight',         'imps_interval2_weight'),
    ORMFloat        ('imps_interval3_weight',         'imps_interval3_weight'),
    ORMFloat        ('keyword_ctr_level',             'keyword_ctr_level'),
    ORMFloat        ('keyword_tow_level',             'keyword_tow_level'),
    ORMFloat        ('kwtg_ctr_default',              'kwtg_ctr_default'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('pub_ctr_default',               'pub_ctr_default'),
    ORMFloat        ('pub_ctr_level',                 'pub_ctr_level'),
    ORMFloat        ('site_ctr_level',                'site_ctr_level'),
    ORMFloat        ('sys_ctr_level',                 'sys_ctr_level'),
    ORMFloat        ('sys_kwtg_ctr_level',            'sys_kwtg_ctr_level'),
    ORMFloat        ('sys_tow_level',                 'sys_tow_level'),
    ORMFloat        ('tag_ctr_level',                 'tag_ctr_level'),
    ORMFloat        ('tg_tow_level',                  'tg_tow_level'),
    ORMFloat        ('tow_raw',                       'tow_raw'),
    ORMTimestamp    ('version',                       'version'),
  ]

currency = Object('currency', 'Currency', True)
currency.id = PQIndex([ORMInt('currency_id', 'currency_id')], 'currency_currency_id_seq')
currency.fields = [
    ORMString       ('currency_code',                 'currency_code'),
    ORMFloat        ('fraction_digits',               'fraction_digits'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('source',                        'source'),
    ORMTimestamp    ('version',                       'version'),
  ]

currencyexchange = Object('currencyexchange', 'CurrencyExchange', True)
currencyexchange.id = PQIndex([ORMInt('currency_exchange_id', 'id')], 'currencyexchange_currency_exchange_id_seq')
currencyexchange.fields = [
    ORMTimestamp    ('effective_date',                'effective_date'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

currencyexchangerate = Object('currencyexchangerate', 'Currencyexchangerate', True)
currencyexchangerate.id = PQIndex([
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('currency_id',                   'currency_id'),
  ] )
currencyexchangerate.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMTimestamp    ('last_updated_date',             'last_updated_date'),
    ORMFloat        ('rate',                          'rate'),
  ]

devicechannelcountstats = Object('devicechannelcountstats', 'Devicechannelcountstats', False)
devicechannelcountstats.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
    ORMString       ('channel_type',                  'channel_type'),
    ORMFloat        ('channel_count',                 'channel_count'),
  ] )
devicechannelcountstats.fields = [
    ORMFloat        ('users_count',                   'users_count'),
  ]

discoverchannelstate = Object('discoverchannelstate', 'Discoverchannelstate', False)
discoverchannelstate.id = PQIndex([ORMInt('channel_id', 'channel_id')])
discoverchannelstate.fields = [
    ORMString       ('annotation_stem',               'annotation_stem'),
    ORMInt          ('daily_news',                    'daily_news'),
    ORMTimestamp    ('last_update',                   'last_update'),
    ORMInt          ('total_news',                    'total_news'),
  ]

displaystatus = Object('displaystatus', 'Displaystatus', False)
displaystatus.id = PQIndex([
    ORMInt          ('display_status_id',             'display_status_id'),
    ORMInt          ('object_type_id',                'object_type_id'),
  ] )
displaystatus.fields = [
    ORMString       ('adserver_status',               'adserver_status'),
    ORMString       ('description',                   'description'),
    ORMString       ('disp_status',                   'disp_status'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

dynamicresources = Object('dynamicresources', 'Dynamicresources', False)
dynamicresources.id = PQIndex([
    ORMString       ('key',                           'key'),
    ORMString       ('lang',                          'lang'),
  ] )
dynamicresources.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('value',                         'value'),
  ]

expressionperformance = Object('expressionperformance', 'Expressionperformance', False)
expressionperformance.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMString       ('expression',                    'expression'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
expressionperformance.fields = [
    ORMFloat        ('actions',                       'actions'),
    ORMFloat        ('clicks',                        'clicks'),
    ORMFloat        ('imps_verified',                 'imps_verified'),
  ]

expressionusedchannel = Object('expressionusedchannel', 'Expressionusedchannel', False)
expressionusedchannel.id = PQIndex([
    ORMInt          ('expression_channel_id',         'expression_channel_id'),
    ORMInt          ('used_channel_id',               'used_channel_id'),
  ] )
expressionusedchannel.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

feed = Object('feed', 'Feed', True)
feed.id = PQIndex([ORMInt('feed_id', 'feed_id')], 'feed_feed_id_seq')
feed.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('url',                           'url'),
  ]

feedstate = Object('feedstate', 'Feedstate', True)
feedstate.id = PQIndex([ORMInt('feed_id', 'feed_id')])
feedstate.fields = [
    ORMInt          ('items',                         'items'),
    ORMTimestamp    ('last_update',                   'last_update'),
  ]

fraudcondition = Object('fraudcondition', 'Fraudcondition', False)
fraudcondition.id = PQIndex([ORMInt('fraud_condition_id', 'fraud_condition_id')], 'fraudcondition_fraud_condition_id_seq')
fraudcondition.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('limit',                         'limit'),
    ORMFloat        ('period',                        'period'),
    ORMString       ('type',                          'type'),
    ORMTimestamp    ('version',                       'version'),
  ]

freqcap = Object('freqcap', 'FreqCap', True)
freqcap.id = PQIndex([ORMInt('freq_cap_id', 'id')], 'freqcap_freq_cap_id_seq')
freqcap.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('life_count',                    'life_count'),
    ORMFloat        ('period',                        'period'),
    ORMTimestamp    ('version',                       'version'),
    ORMFloat        ('window_count',                  'window_count'),
    ORMFloat        ('window_length',                 'window_length'),
  ]

globalcolouserstats = Object('globalcolouserstats', 'Globalcolouserstats', False)
globalcolouserstats.id = PQIndex([
    ORMDate         ('global_sdate',                  'global_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('create_date',                   'create_date'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
globalcolouserstats.fields = [
    ORMInt          ('network_unique_users',          'network_unique_users'),
    ORMInt          ('profiling_unique_users',        'profiling_unique_users'),
    ORMInt          ('unique_hids',                   'unique_hids'),
    ORMInt          ('unique_users',                  'unique_users'),
  ]

historyctrstatshourly = Object('historyctrstatshourly', 'Historyctrstatshourly', False)
historyctrstatshourly.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('ctr_reset_id',                  'ctr_reset_id'),
  ] )
historyctrstatshourly.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
  ]

historyctrstatstotal = Object('historyctrstatstotal', 'Historyctrstatstotal', False)
historyctrstatstotal.id = PQIndex([
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('ctr_reset_id',                  'ctr_reset_id'),
  ] )
historyctrstatstotal.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
  ]

historyctrstatstow = Object('historyctrstatstow', 'Historyctrstatstow', False)
historyctrstatstow.id = PQIndex([
    ORMInt          ('day_of_week',                   'day_of_week'),
    ORMInt          ('hour',                          'hour'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('ctr_reset_id',                  'ctr_reset_id'),
  ] )
historyctrstatstow.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
  ]

iasstatshourly = DualObject('iasstatshourly', 'Iasstatshourly', False)
iasstatshourly.id = PQIndex([
  ] )
iasstatshourly.fields = [
    ORMInt          ('adultsc',                       'adultsc'),
    ORMInt          ('alcoholsc',                     'alcoholsc'),
    ORMString       ('browser',                       'browser'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('confidence',                    'confidence'),
    ORMInt          ('count',                         'count'),
    ORMString       ('country_code',                  'country_code'),
    ORMString       ('dma',                           'dma'),
    ORMInt          ('downloadsc',                    'downloadsc'),
    ORMInt          ('drugsc',                        'drugsc'),
    ORMString       ('full_url',                      'full_url'),
    ORMInt          ('hatesc',                        'hatesc'),
    ORMString       ('iabcategories',                 'iabcategories'),
    ORMFloat        ('inview15s',                     'inview15s'),
    ORMFloat        ('inview1s',                      'inview1s'),
    ORMFloat        ('inview5s',                      'inview5s'),
    ORMBool         ('inviewabilitysample',           'inviewabilitysample'),
    ORMString       ('inviewfield',                   'inviewfield'),
    ORMFloat        ('inviewtime',                    'inviewtime'),
    ORMFloat        ('maxfractioninview',             'maxfractioninview'),
    ORMString       ('noviewabilityreason',           'noviewabilityreason'),
    ORMInt          ('offensivelanguagesc',           'offensivelanguagesc'),
    ORMString       ('os',                            'os'),
    ORMString       ('platform',                      'platform'),
    ORMDate         ('received_date',                 'received_date'),
    ORMInt          ('received_hour',                 'received_hour'),
    ORMInt          ('received_month',                'received_month'),
    ORMInt          ('received_year',                 'received_year'),
    ORMFloat        ('sadrisk',                       'sadrisk'),
    ORMString       ('state',                         'state'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('tag_size_id',                   'tag_size_id'),
    ORMString       ('traqbucket',                    'traqbucket'),
    ORMString       ('useragentstr',                  'useragentstr'),
    ORMInt          ('viewabilityenum',               'viewabilityenum'),
  ]

insertionorder = Object('insertionorder', 'Insertionorder', False)
insertionorder.id = PQIndex([ORMInt('io_id', 'io_id')], 'insertionorder_io_id_seq')
insertionorder.fields = [
    ORMInt          ('account_id',                    'account_id', 'account'),
    ORMFloat        ('amount',                        'amount'),
    ORMString       ('io_number',                     'io_number'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('notes',                         'notes'),
    ORMString       ('po_number',                     'po_number'),
    ORMString       ('probability',                   'probability'),
    ORMTimestamp    ('version',                       'version'),
  ]

ispcampaignstatsdaily = Object('ispcampaignstatsdaily', 'Ispcampaignstatsdaily', False)
ispcampaignstatsdaily.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMString       ('ccg_rate_type',                 'ccg_rate_type'),
    ORMInt          ('size_id',                       'size_id'),
  ] )
ispcampaignstatsdaily.fields = [
    ORMFloat        ('adv_amount_isp',                'adv_amount_isp'),
    ORMFloat        ('clicks',                        'clicks'),
    ORMFloat        ('credited_adv_amount_isp',       'credited_adv_amount_isp'),
    ORMInt          ('credited_clicks',               'credited_clicks'),
    ORMInt          ('credited_imps',                 'credited_imps'),
    ORMFloat        ('imps',                          'imps'),
  ]

ispstatsdaily = Object('ispstatsdaily', 'Ispstatsdaily', False)
ispstatsdaily.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMBool         ('hid_profile',                   'hid_profile'),
  ] )
ispstatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('credited_clicks',               'credited_clicks'),
    ORMInt          ('credited_imps',                 'credited_imps'),
    ORMFloat        ('credited_isp_amount',           'credited_isp_amount'),
    ORMFloat        ('credited_isp_amount_global',    'credited_isp_amount_global'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('requests',                      'requests'),
  ]

ispstatsdailycountry = Object('ispstatsdailycountry', 'Ispstatsdailycountry', False)
ispstatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('colo_rate_id',                  'colo_rate_id'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
  ] )
ispstatsdailycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_amount_isp',                'adv_amount_isp'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMFloat        ('adv_comm_amount_isp',           'adv_comm_amount_isp'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_amount_isp',                'pub_amount_isp'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMFloat        ('pub_comm_amount_isp',           'pub_comm_amount_isp'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
  ]

ispstatstotal = Object('ispstatstotal', 'Ispstatstotal', False)
ispstatstotal.id = PQIndex([
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('country_code',                  'country_code'),
  ] )
ispstatstotal.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('requests',                      'requests'),
    ORMFloat        ('slot_imps',                     'slot_imps'),
  ]

keywordinventory = Object('keywordinventory', 'Keywordinventory', False)
keywordinventory.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('position',                      'position'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMInt          ('last_page_match',               'last_page_match'),
    ORMInt          ('last_search_match',             'last_search_match'),
    ORMInt          ('matched_page_1h',               'matched_page_1h'),
    ORMInt          ('matched_page_24h',              'matched_page_24h'),
    ORMInt          ('matched_page_7d',               'matched_page_7d'),
    ORMInt          ('matched_search_1h',             'matched_search_1h'),
    ORMInt          ('matched_search_24h',            'matched_search_24h'),
    ORMInt          ('matched_search_7d',             'matched_search_7d'),
    ORMInt          ('ccg_imps_24h',                  'ccg_imps_24h'),
    ORMInt          ('creative_imps_24h',             'creative_imps_24h'),
    ORMInt          ('keyword_imps_24h',              'keyword_imps_24h'),
  ] )
keywordinventory.fields = [
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
  ]

nbostats = Object('nbostats', 'Nbostats', False)
nbostats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('hid_class_name',                'hid_class_name'),
    ORMString       ('allocated',                     'allocated'),
    ORMString       ('network_status',                'network_status'),
  ] )
nbostats.fields = [
    ORMFloat        ('have_uid_optout',               'have_uid_optout'),
    ORMFloat        ('invited',                       'invited'),
    ORMFloat        ('total',                         'total'),
  ]

objecttype = Object('objecttype', 'Objecttype', False)
objecttype.id = PQIndex([ORMInt('object_type_id', 'object_type_id')])
objecttype.fields = [
    ORMString       ('class',                         'class'),
    ORMString       ('name',                          'name'),
    ORMString       ('table_name',                    'table_name'),
  ]

foros_applied_patches = Object('oix_applied_patches', 'Oix_applied_patches', False)
foros_applied_patches.id = PQIndex([ORMString('patch_name', 'patch_name')])
foros_applied_patches.fields = [
    ORMTimestamp    ('end_date',                      'end_date'),
    ORMTimestamp    ('start_date',                    'start_date'),
    ORMString       ('status',                        'status'),
  ]

foros_timed_services = Object('oix_timed_services', 'Oix_timed_services', False)
foros_timed_services.id = PQIndex([ORMString('service_id', 'service_id')])
foros_timed_services.fields = [
    ORMString       ('instance_id',                   'instance_id'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMTimestamp    ('version',                       'version'),
  ]

optionenumvalue = Object('optionenumvalue', 'Optionenumvalue', False)
optionenumvalue.id = PQIndex([ORMInt('option_enum_value_id', 'option_enum_value_id')], 'optionenumvalue_option_enum_value_id_seq')
optionenumvalue.fields = [
    ORMBool         ('is_default',                    'is_default'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMInt          ('option_id',                     'option_id'),
    ORMString       ('value',                         'value'),
    ORMTimestamp    ('version',                       'version'),
  ]

optionfiletype = Object('optionfiletype', 'Optionfiletype', False)
optionfiletype.id = PQIndex([ORMInt('optionfiletype_id', 'optionfiletype_id')], 'optionfiletype_optionfiletype_id_seq')
optionfiletype.fields = [
    ORMString       ('file_type',                     'file_type'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('option_id',                     'option_id'),
  ]

optiongroup = Object('optiongroup', 'Optiongroup', False)
optiongroup.id = PQIndex([ORMInt('option_group_id', 'option_group_id')], 'optiongroup_option_group_id_seq')
optiongroup.fields = [
    ORMString       ('availability',                  'availability'),
    ORMString       ('collapsibility',                'collapsibility'),
    ORMString       ('label',                         'label'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMInt          ('size_id',                       'size_id', 'creativesize'),
    ORMFloat        ('sort_order',                    'sort_order'),
    ORMInt          ('template_id',                   'template_id', 'template'),
    ORMString       ('type',                          'type'),
    ORMTimestamp    ('version',                       'version'),
  ]

options = Object('options', 'Options', False)
options.id = PQIndex([ORMInt('option_id', 'option_id')], 'options_option_id_seq')
options.fields = [
    ORMString       ('default_value',                 'default_value'),
    ORMString       ('label',                         'label'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('max_value',                     'max_value'),
    ORMFloat        ('min_value',                     'min_value'),
    ORMString       ('name',                          'name'),
    ORMInt          ('option_group_id',               'option_group_id', 'optiongroup'),
    ORMFloat        ('recursive_tokens',              'recursive_tokens'),
    ORMBool         ('required',                      'required'),
    ORMInt          ('size_id',                       'size_id'),
    ORMFloat        ('sort_order',                    'sort_order'),
    ORMInt          ('template_id',                   'template_id'),
    ORMString       ('token',                         'token'),
    ORMString       ('type',                          'type'),
    ORMTimestamp    ('version',                       'version'),
  ]

optoutstats = Object('optoutstats', 'Optoutstats', False)
optoutstats.id = PQIndex([
    ORMTimestamp    ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('referrer',                      'referrer'),
    ORMString       ('operation',                     'operation'),
    ORMFloat        ('status',                        'status'),
    ORMString       ('test',                          'test'),
  ] )
optoutstats.fields = [
    ORMFloat        ('count',                         'count'),
  ]

oracle_job = Object('oracle_job', 'Oracle_job', False)
oracle_job.id = PQIndex([ORMInt('id', 'id')])
oracle_job.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
  ]

pageloadsdaily = Object('pageloadsdaily', 'Pageloadsdaily', False)
pageloadsdaily.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('site_id',                       'site_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMString       ('tag_group',                     'tag_group'),
  ] )
pageloadsdaily.fields = [
    ORMInt          ('page_loads',                    'page_loads'),
    ORMInt          ('utilized_page_loads',           'utilized_page_loads'),
  ]

passbackstats = Object('passbackstats', 'Passbackstats', False)
passbackstats.id = PQIndex([
    ORMInt          ('colo_id',                       'colo_id'),
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
  ] )
passbackstats.fields = [
    ORMInt          ('requests',                      'requests'),
  ]

placementblacklist = Object('placementblacklist', 'Placementblacklist', False)
placementblacklist.id = PQIndex([ORMInt('channel_trigger_id', 'channel_trigger_id')])
placementblacklist.fields = [
    ORMInt          ('channel_id',                    'channel_id'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('reason',                        'reason'),
    ORMTimestamp    ('timestamp_added',               'timestamp_added'),
    ORMString       ('url',                           'url'),
    ORMInt          ('user_id',                       'user_id'),
  ]

plan_log = DualObject('plan_log', 'Plan_log', False)
plan_log.id = PQIndex([
  ] )
plan_log.fields = [
    ORMString       ('key',                           'key'),
    ORMString       ('plan',                          'plan'),
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMString       ('sql',                           'sql'),
  ]

platform = Object('platform', 'Platform', False)
platform.id = PQIndex([ORMInt('platform_id', 'platform_id')])
platform.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('type',                          'type'),
  ]

platformdetector = Object('platformdetector', 'Platformdetector', False)
platformdetector.id = PQIndex([ORMInt('platform_detector_id', 'platform_detector_id')])
platformdetector.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('match_marker',                  'match_marker'),
    ORMString       ('match_regexp',                  'match_regexp'),
    ORMString       ('output_regexp',                 'output_regexp'),
    ORMInt          ('platform_id',                   'platform_id', 'platform'),
    ORMInt          ('priority',                      'priority'),
  ]

policy = Object('policy', 'Policy', False)
policy.id = PQIndex([ORMInt('perm_id', 'perm_id')], 'policy_perm_id_seq')
policy.fields = [
    ORMString       ('action_type',                   'action_type'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('parameter',                     'parameter'),
    ORMString       ('permission_type',               'permission_type'),
    ORMInt          ('user_role_id',                  'user_role_id', 'userrole'),
    ORMTimestamp    ('version',                       'version'),
  ]

publisherinventory = Object('publisherinventory', 'Publisherinventory', False)
publisherinventory.id = PQIndex([
    ORMDate         ('pub_sdate',                     'pub_sdate'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMFloat        ('cpm',                           'cpm'),
  ] )
publisherinventory.fields = [
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('requests',                      'requests'),
    ORMFloat        ('revenue',                       'revenue'),
  ]

publisherstatsdaily = Object('publisherstatsdaily', 'Publisherstatsdaily', False)
publisherstatsdaily.id = PQIndex([
    ORMDate         ('pub_sdate',                     'pub_sdate'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMInt          ('site_rate_id',                  'site_rate_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMBool         ('walled_garden',                 'walled_garden'),
  ] )
publisherstatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('invalid_clicks',                'invalid_clicks'),
    ORMFloat        ('invalid_imps',                  'invalid_imps'),
    ORMFloat        ('invalid_requests',              'invalid_requests'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('pub_credited_imps',             'pub_credited_imps'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('site_id',                       'site_id'),
    ORMInt          ('tag_pricing_id',                'tag_pricing_id'),
  ]

publisherstatsdailycountry = Object('publisherstatsdailycountry', 'Publisherstatsdailycountry', False)
publisherstatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('site_rate_id',                  'site_rate_id'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
  ] )
publisherstatsdailycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_amount_pub',                'adv_amount_pub'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMFloat        ('adv_comm_amount_pub',           'adv_comm_amount_pub'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMFloat        ('isp_amount_pub',                'isp_amount_pub'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('pub_credited_imps',             'pub_credited_imps'),
    ORMInt          ('requests',                      'requests'),
  ]

publisherstatstotal = Object('publisherstatstotal', 'Publisherstatstotal', False)
publisherstatstotal.id = PQIndex([
    ORMInt          ('tag_id',                        'tag_id'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('size_id',                       'size_id'),
  ] )
publisherstatstotal.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMInt          ('pub_credited_imps',             'pub_credited_imps'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('site_id',                       'site_id'),
    ORMFloat        ('slot_imps',                     'slot_imps'),
  ]

requeststatsdailycountry = Object('requeststatsdailycountry', 'Requeststatsdailycountry', False)
requeststatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('ccg_rate_id',                   'ccg_rate_id'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('colo_rate_id',                  'colo_rate_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMInt          ('site_rate_id',                  'site_rate_id'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMInt          ('position',                      'position'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
  ] )
requeststatsdailycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMFloat        ('adv_invoice_comm_amount',       'adv_invoice_comm_amount'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('site_id',                       'site_id'),
  ]

requeststatsdailyisp = Object('requeststatsdailyisp', 'Requeststatsdailyisp', False)
requeststatsdailyisp.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('ccg_rate_id',                   'ccg_rate_id'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('colo_rate_id',                  'colo_rate_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMInt          ('site_rate_id',                  'site_rate_id'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMInt          ('position',                      'position'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
  ] )
requeststatsdailyisp.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMFloat        ('adv_invoice_comm_amount',       'adv_invoice_comm_amount'),
    ORMInt          ('campaign_id',                   'campaign_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('site_id',                       'site_id'),
  ]

requeststatshourly = Object('requeststatshourly', 'Requeststatshourly', False)
requeststatshourly.id = PQIndex([
    ORMTimestamp    ('stimestamp',                    'stimestamp'),
    ORMInt          ('ccg_rate_id',                   'ccg_rate_id'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('isp_account_id',                'isp_account_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('colo_rate_id',                  'colo_rate_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMInt          ('site_rate_id',                  'site_rate_id'),
    ORMBool         ('hid_profile',                   'hid_profile'),
    ORMBool         ('test',                          'test'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMString       ('user_status',                   'user_status'),
    ORMString       ('country_code',                  'country_code'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMInt          ('position',                      'position'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
  ] )
requeststatshourly.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('adv_account_id',                'adv_account_id'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMFloat        ('adv_invoice_comm_amount',       'adv_invoice_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('country_hour',                  'country_hour'),
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('site_id',                       'site_id'),
  ]

requeststatsmonthlycountry = Object('requeststatsmonthlycountry', 'Requeststatsmonthlycountry', False)
requeststatsmonthlycountry.id = PQIndex([
    ORMString       ('country_code',                  'country_code'),
    ORMBool         ('fraud_correction',              'fraud_correction'),
    ORMBool         ('walled_garden',                 'walled_garden'),
    ORMString       ('user_status',                   'user_status'),
    ORMInt          ('ccg_rate_id',                   'ccg_rate_id'),
    ORMInt          ('cc_id',                         'cc_id'),
    ORMInt          ('colo_rate_id',                  'colo_rate_id'),
    ORMInt          ('currency_exchange_id',          'currency_exchange_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('size_id',                       'size_id'),
    ORMInt          ('site_rate_id',                  'site_rate_id'),
    ORMInt          ('num_shown',                     'num_shown'),
    ORMInt          ('position',                      'position'),
    ORMInt          ('device_channel_id',             'device_channel_id'),
  ] )
requeststatsmonthlycountry.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMFloat        ('adv_amount',                    'adv_amount'),
    ORMFloat        ('adv_amount_global',             'adv_amount_global'),
    ORMFloat        ('adv_comm_amount',               'adv_comm_amount'),
    ORMFloat        ('adv_comm_amount_global',        'adv_comm_amount_global'),
    ORMFloat        ('adv_invoice_comm_amount',       'adv_invoice_comm_amount'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
    ORMInt          ('imps',                          'imps'),
    ORMFloat        ('isp_amount',                    'isp_amount'),
    ORMFloat        ('isp_amount_global',             'isp_amount_global'),
    ORMInt          ('passbacks',                     'passbacks'),
    ORMFloat        ('pub_amount',                    'pub_amount'),
    ORMFloat        ('pub_amount_global',             'pub_amount_global'),
    ORMFloat        ('pub_comm_amount',               'pub_comm_amount'),
    ORMFloat        ('pub_comm_amount_global',        'pub_comm_amount_global'),
    ORMString       ('pub_country_code',              'pub_country_code'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('site_id',                       'site_id'),
  ]

rtbcategory = Object('rtbcategory', 'Rtbcategory', False)
rtbcategory.id = PQIndex([ORMInt('rtb_category_id', 'rtb_category_id')], 'rtbcategory_rtb_category_id_seq')
rtbcategory.fields = [
    ORMInt          ('creative_category_id',          'creative_category_id'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('rtb_category_key',              'rtb_category_key'),
    ORMInt          ('rtb_id',                        'rtb_id'),
  ]

rtbconnector = Object('rtbconnector', 'Rtbconnector', False)
rtbconnector.id = PQIndex([ORMInt('rtb_id', 'rtb_id')], 'rtbconnector_rtb_id_seq')
rtbconnector.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
  ]

searchengine = Object('searchengine', 'SearchEngine', True)
searchengine.id = PQIndex([ORMInt('search_engine_id', 'search_engine_id')], 'searchengine_search_engine_id_seq')
searchengine.fields = [
    ORMFloat        ('decoding_depth',                'decoding_depth'),
    ORMString       ('encoding',                      'encoding'),
    ORMString       ('host',                          'host'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('post_encoding',                 'post_encoding'),
    ORMString       ('regexp',                        'regexp'),
    ORMTimestamp    ('version',                       'version'),
  ]

searchenginestatsdaily = Object('searchenginestatsdaily', 'Searchenginestatsdaily', False)
searchenginestatsdaily.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('search_engine_id',              'search_engine_id'),
    ORMString       ('host_name',                     'host_name'),
  ] )
searchenginestatsdaily.fields = [
    ORMInt          ('hits',                          'hits'),
    ORMInt          ('hits_empty_page',               'hits_empty_page'),
  ]

searchtermstatsdaily = Object('searchtermstatsdaily', 'Searchtermstatsdaily', False)
searchtermstatsdaily.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('search_term_id',                'search_term_id'),
  ] )
searchtermstatsdaily.fields = [
    ORMInt          ('hits',                          'hits'),
  ]

site = Object('site', 'Site', True)
site.id = PQIndex([ORMInt('site_id', 'id')], 'site_site_id_seq')
site.fields = [
    ORMInt          ('account_id',                    'account', 'account'),
    ORMInt          ('display_status_id',             'display_status_id', default = "1"),
    ORMInt          ('flags',                         'flags'),
    ORMInt          ('freq_cap_id',                   'freq_cap', 'freqcap'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMFloat        ('no_ads_timeout',                'no_ads_timeout', default = "0"),
    ORMString       ('notes',                         'notes'),
    ORMTimestamp    ('qa_date',                       'qa_date'),
    ORMString       ('qa_description',                'qa_description'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMInt          ('qa_user_id',                    'qa_user_id'),
    ORMInt          ('site_category_id',              'site_category_id', 'sitecategory'),
    ORMString       ('site_url',                      'site_url'),
    ORMString       ('status',                        'status'),
    ORMTimestamp    ('version',                       'version'),
  ]

sitecategory = Object('sitecategory', 'Sitecategory', True)
sitecategory.id = PQIndex([ORMInt('site_category_id', 'site_category_id')], 'sitecategory_site_category_id_seq')
sitecategory.fields = [
    ORMString       ('country_code',                  'country_code', 'country'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('version',                       'version'),
  ]

sitechannelstats = Object('sitechannelstats', 'Sitechannelstats', False)
sitechannelstats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('channel_id',                    'channel_id'),
  ] )
sitechannelstats.fields = [
    ORMFloat        ('adv_revenue',                   'adv_revenue'),
    ORMFloat        ('imps',                          'imps'),
    ORMFloat        ('pub_revenue',                   'pub_revenue'),
  ]

sitecreativeapproval = Object('sitecreativeapproval', 'SiteCreativeApproval', True)
sitecreativeapproval.id = PQIndex([
    ORMInt          ('creative_id',                   'creative_id'),
    ORMInt          ('site_id',                       'site_id'),
  ] )
sitecreativeapproval.fields = [
    ORMString       ('approval',                      'approval'),
    ORMTimestamp    ('approval_date',                 'approval_date'),
    ORMString       ('feedback',                      'feedback'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('reject_reason_id',              'reject_reason_id'),
  ]

sitecreativecategoryexclusion = Object('sitecreativecategoryexclusion', 'SiteCreativeCategoryExclusion', True)
sitecreativecategoryexclusion.id = PQIndex([
    ORMInt          ('creative_category_id',          'creative_category_id'),
    ORMInt          ('site_id',                       'site_id'),
  ] )
sitecreativecategoryexclusion.fields = [
    ORMString       ('approval',                      'approval'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

siterate = Object('siterate', 'SiteRate', True)
siterate.id = PQIndex([ORMInt('site_rate_id', 'id')], 'siterate_site_rate_id_seq')
siterate.fields = [
    ORMTimestamp    ('effective_date',                'effective_date', default = "now()"),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('rate',                          'rate'),
    ORMString       ('rate_type',                     'rate_type'),
    ORMInt          ('tag_pricing_id',                'tag_pricing', 'tagpricing'),
  ]

siteuserstats = Object('siteuserstats', 'Siteuserstats', False)
siteuserstats.id = PQIndex([
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('site_id',                       'site_id'),
    ORMDate         ('last_appearance_date',          'last_appearance_date'),
  ] )
siteuserstats.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

siteuserstatsrunning = DualObject('siteuserstatsrunning', 'Siteuserstatsrunning', False)
siteuserstatsrunning.id = PQIndex([
  ] )
siteuserstatsrunning.fields = [
    ORMFloat        ('daily_unique_users',            'daily_unique_users'),
    ORMDate         ('isp_sdate',                     'isp_sdate'),
    ORMFloat        ('monthly_unique_users',          'monthly_unique_users'),
    ORMFloat        ('new_unique_users',              'new_unique_users'),
    ORMFloat        ('running_unique_users',          'running_unique_users'),
    ORMInt          ('site_id',                       'site_id'),
    ORMFloat        ('weekly_unique_users',           'weekly_unique_users'),
  ]

siteuserstatstotal = Object('siteuserstatstotal', 'Siteuserstatstotal', False)
siteuserstatstotal.id = PQIndex([ORMInt('site_id', 'site_id')])
siteuserstatstotal.fields = [
    ORMFloat        ('unique_users',                  'unique_users'),
  ]

sizetype = Object('sizetype', 'Sizetype', True)
sizetype.id = PQIndex([ORMInt('size_type_id', 'size_type_id')], 'sizetype_size_type_id_seq')
sizetype.fields = [
    ORMFloat        ('flags',                         'flags'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('tag_templ_brpb_file',           'tag_templ_brpb_file'),
    ORMString       ('tag_templ_iest_file',           'tag_templ_iest_file'),
    ORMString       ('tag_templ_iframe_file',         'tag_templ_iframe_file'),
    ORMString       ('tag_templ_preview_file',        'tag_templ_preview_file'),
    ORMString       ('tag_template_file',             'tag_template_file'),
    ORMTimestamp    ('version',                       'version'),
  ]

tagauctionsettings = Object('tagauctionsettings', 'Tagauctionsettings', False)
tagauctionsettings.id = PQIndex([ORMInt('tag_id', 'tag_id')])
tagauctionsettings.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMFloat        ('max_ecpm_share',                'max_ecpm_share'),
    ORMFloat        ('prop_probability_share',        'prop_probability_share'),
    ORMFloat        ('random_share',                  'random_share'),
    ORMTimestamp    ('version',                       'version'),
  ]

tagauctionstats = Object('tagauctionstats', 'Tagauctionstats', False)
tagauctionstats.id = PQIndex([
    ORMDate         ('pub_sdate',                     'pub_sdate'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('auction_ccg_count',             'auction_ccg_count'),
  ] )
tagauctionstats.fields = [
    ORMInt          ('requests',                      'requests'),
  ]

tagcontentcategory = Object('tagcontentcategory', 'Tagcontentcategory', False)
tagcontentcategory.id = PQIndex([
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('content_category_id',           'content_category_id'),
  ] )
tagcontentcategory.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

tagctroverride = Object('tagctroverride', 'Tagctroverride', False)
tagctroverride.id = PQIndex([ORMInt('tag_id', 'tag_id')])
tagctroverride.fields = [
    ORMFloat        ('adjustment',                    'adjustment'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

tagoptgroupstate = Object('tagoptgroupstate', 'Tagoptgroupstate', False)
tagoptgroupstate.id = PQIndex([
    ORMInt          ('option_group_id',               'option_group_id'),
    ORMInt          ('tag_id',                        'tag_id'),
  ] )
tagoptgroupstate.fields = [
    ORMBool         ('collapsed',                     'collapsed'),
    ORMBool         ('enabled',                       'enabled'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMTimestamp    ('version',                       'version'),
  ]

tagoptionvalue = Object('tagoptionvalue', 'Tagoptionvalue', False)
tagoptionvalue.id = PQIndex([
    ORMInt          ('option_id',                     'option_id'),
    ORMInt          ('tag_id',                        'tag_id'),
  ] )
tagoptionvalue.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('value',                         'value'),
    ORMTimestamp    ('version',                       'version'),
  ]

tagpositionstats = DualObject('tagpositionstats', 'Tagpositionstats', False)
tagpositionstats.id = PQIndex([
  ] )
tagpositionstats.fields = [
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('left_offset',                   'left_offset'),
    ORMDate         ('pub_sdate',                     'pub_sdate'),
    ORMInt          ('requests',                      'requests'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMString       ('test',                          'test'),
    ORMInt          ('top_offset',                    'top_offset'),
    ORMInt          ('visibility',                    'visibility'),
  ]

tagpricing = Object('tagpricing', 'TagPricing', True)
tagpricing.id = PQIndex([ORMInt('tag_pricing_id', 'id')], 'tagpricing_tag_pricing_id_seq')
tagpricing.fields = [
    ORMString       ('ccg_rate_type',                 'ccg_rate_type'),
    ORMString       ('ccg_type',                      'ccg_type'),
    ORMString       ('country_code',                  'country_code', 'country'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('site_rate_id',                  'site_rate', 'siterate'),
    ORMString       ('status',                        'status'),
    ORMInt          ('tag_id',                        'tag', 'tags'),
    ORMTimestamp    ('version',                       'version'),
  ]

tags = Object('tags', 'Tags', True)
tags.id = PQIndex([ORMInt('tag_id', 'id')], 'tags_tag_id_seq')
tags.fields = [
    ORMBool         ('allow_expandable',              'allow_expandable', default = "'N'"),
    ORMInt          ('flags',                         'flags', default = "0"),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('marketplace',                   'marketplace', default = "'WG'"),
    ORMString       ('name',                          'name'),
    ORMString       ('passback',                      'passback'),
    ORMString       ('passback_code',                 'passback_code'),
    ORMString       ('passback_type',                 'passback_type', default = "'HTML_URL'"),
    ORMInt          ('site_id',                       'site', 'site'),
    ORMInt          ('size_type_id',                  'size_type_id'),
    ORMString       ('status',                        'status', default = "'A'"),
    ORMTimestamp    ('version',                       'version'),
  ]

tagscreativecategoryexclusion = Object('tagscreativecategoryexclusion', 'Tagscreativecategoryexclusion', True)
tagscreativecategoryexclusion.id = PQIndex([
    ORMInt          ('creative_category_id',          'creative_category_id'),
    ORMInt          ('tag_id',                        'tag_id'),
  ] )
tagscreativecategoryexclusion.fields = [
    ORMString       ('approval',                      'approval'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

tag_tagsize = Object('tag_tagsize', 'Tag_tagsize', True)
tag_tagsize.id = PQIndex([
    ORMInt          ('size_id',                       'size_id'),
    ORMInt          ('tag_id',                        'tag_id'),
  ] )
tag_tagsize.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

template = Object('template', 'Template', True)
template.id = PQIndex([ORMInt('template_id', 'template_id')], 'template_template_id_seq')
template.fields = [
    ORMBool         ('expandable',                    'expandable', default = "'N'"),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('status',                        'status'),
    ORMString       ('template_type',                 'template_type', default = "'CREATIVE'"),
    ORMTimestamp    ('version',                       'version'),
  ]

templatefile = Object('templatefile', 'Templatefile', True)
templatefile.id = PQIndex([ORMInt('template_file_id', 'template_file_id')], 'templatefile_template_file_id_seq')
templatefile.fields = [
    ORMInt          ('app_format_id',                 'app_format_id', 'appformat'),
    ORMInt          ('flags',                         'flags'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('size_id',                       'size_id', 'creativesize'),
    ORMString       ('template_file',                 'template_file'),
    ORMInt          ('template_id',                   'template_id', 'template'),
    ORMString       ('template_type',                 'template_type'),
    ORMTimestamp    ('version',                       'version'),
  ]

thirdpartycreative = Object('thirdpartycreative', 'Thirdpartycreative', False)
thirdpartycreative.id = PQIndex([
    ORMInt          ('site_id',                       'site_id'),
    ORMInt          ('creative_id',                   'creative_id'),
  ] )
thirdpartycreative.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMBool         ('third_party_approval',          'third_party_approval'),
    ORMString       ('third_party_creative_id',       'third_party_creative_id'),
  ]

timezone = Object('timezone', 'Timezone', False)
timezone.id = PQIndex([ORMInt('timezone_id', 'timezone_id')])
timezone.fields = [
    ORMString       ('description',                   'description'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('stat_suffix',                   'stat_suffix'),
    ORMString       ('tzname',                        'tzname'),
  ]

top100urls = Object('top100urls', 'Top100urls', False)
top100urls.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('channel_id',                    'channel_id'),
    ORMInt          ('colo_id',                       'colo_id'),
  ] )
top100urls.fields = [
    ORMFloat        ('actions_d',                     'actions_d'),
    ORMFloat        ('actions_t',                     'actions_t'),
    ORMFloat        ('active_user_count',             'active_user_count'),
    ORMFloat        ('clicks_d',                      'clicks_d'),
    ORMFloat        ('clicks_t',                      'clicks_t'),
    ORMFloat        ('hits',                          'hits'),
    ORMFloat        ('hits_kws',                      'hits_kws'),
    ORMFloat        ('hits_search_kws',               'hits_search_kws'),
    ORMFloat        ('hits_url_kws',                  'hits_url_kws'),
    ORMFloat        ('hits_urls',                     'hits_urls'),
    ORMFloat        ('impops_no_imp_d',               'impops_no_imp_d'),
    ORMFloat        ('impops_no_imp_t',               'impops_no_imp_t'),
    ORMFloat        ('impops_no_imp_user_count_d',    'impops_no_imp_user_count_d'),
    ORMFloat        ('impops_no_imp_user_count_t',    'impops_no_imp_user_count_t'),
    ORMFloat        ('impops_no_imp_value_d',         'impops_no_imp_value_d'),
    ORMFloat        ('impops_no_imp_value_t',         'impops_no_imp_value_t'),
    ORMFloat        ('impops_user_count_d',           'impops_user_count_d'),
    ORMFloat        ('impops_user_count_t',           'impops_user_count_t'),
    ORMFloat        ('imps_d',                        'imps_d'),
    ORMFloat        ('imps_other_d',                  'imps_other_d'),
    ORMFloat        ('imps_other_t',                  'imps_other_t'),
    ORMFloat        ('imps_other_user_count_d',       'imps_other_user_count_d'),
    ORMFloat        ('imps_other_user_count_t',       'imps_other_user_count_t'),
    ORMFloat        ('imps_other_value_d',            'imps_other_value_d'),
    ORMFloat        ('imps_other_value_t',            'imps_other_value_t'),
    ORMFloat        ('imps_t',                        'imps_t'),
    ORMFloat        ('imps_user_count_d',             'imps_user_count_d'),
    ORMFloat        ('imps_user_count_t',             'imps_user_count_t'),
    ORMFloat        ('imps_value_d',                  'imps_value_d'),
    ORMFloat        ('imps_value_t',                  'imps_value_t'),
    ORMFloat        ('revenue_d',                     'revenue_d'),
    ORMFloat        ('revenue_t',                     'revenue_t'),
    ORMFloat        ('sum_ecpm',                      'sum_ecpm'),
    ORMFloat        ('total_user_count',              'total_user_count'),
  ]

triggers = Object('triggers', 'Triggers', True)
triggers.id = PQIndex([ORMInt('trigger_id', 'trigger_id')], 'triggers_trigger_id_seq')
triggers.fields = [
    ORMString       ('channel_type',                  'channel_type'),
    ORMString       ('country_code',                  'country_code'),
    ORMTimestamp    ('created',                       'created', default = "timestamp 'now'"),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('normalized_trigger',            'normalized_trigger'),
    ORMString       ('qa_status',                     'qa_status'),
    ORMString       ('trigger_type',                  'trigger_type'),
    ORMTimestamp    ('version',                       'version', default = "timestamp 'now'"),
  ]

useradvertiser = Object('useradvertiser', 'Useradvertiser', False)
useradvertiser.id = PQIndex([
    ORMInt          ('user_id',                       'user_id'),
    ORMInt          ('account_id',                    'account_id'),
  ] )
useradvertiser.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

useragentstats = Object('useragentstats', 'Useragentstats', False)
useragentstats.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMString       ('user_agent',                    'user_agent'),
  ] )
useragentstats.fields = [
    ORMString       ('channels',                      'channels'),
    ORMString       ('platforms',                     'platforms'),
    ORMFloat        ('requests',                      'requests'),
  ]

userproperty = Object('userproperty', 'Userproperty', False)
userproperty.id = PQIndex([ORMInt('user_property_id', 'user_property_id')], 'userproperty_user_property_id_seq')
userproperty.fields = [
    ORMString       ('name',                          'name'),
    ORMString       ('value',                         'value'),
  ]

userpropertystatsdaily = Object('userpropertystatsdaily', 'Userpropertystatsdaily', False)
userpropertystatsdaily.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('user_property_id',              'user_property_id'),
    ORMString       ('user_status',                   'user_status'),
  ] )
userpropertystatsdaily.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('imps_unverified',               'imps_unverified'),
    ORMInt          ('profiling_requests',            'profiling_requests'),
    ORMInt          ('requests',                      'requests'),
  ]

userpropertystatshourly = Object('userpropertystatshourly', 'Userpropertystatshourly', False)
userpropertystatshourly.id = PQIndex([
    ORMTimestamp    ('stimestamp',                    'stimestamp'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('user_property_id',              'user_property_id'),
    ORMString       ('user_status',                   'user_status'),
  ] )
userpropertystatshourly.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('hour',                          'hour'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('imps_unverified',               'imps_unverified'),
    ORMInt          ('profiling_requests',            'profiling_requests'),
    ORMInt          ('requests',                      'requests'),
    ORMDate         ('sdate',                         'sdate'),
  ]

userpropertystatsmonthly = Object('userpropertystatsmonthly', 'Userpropertystatsmonthly', False)
userpropertystatsmonthly.id = PQIndex([
    ORMDate         ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('user_property_id',              'user_property_id'),
    ORMString       ('user_status',                   'user_status'),
  ] )
userpropertystatsmonthly.fields = [
    ORMInt          ('actions',                       'actions'),
    ORMInt          ('clicks',                        'clicks'),
    ORMInt          ('imps',                          'imps'),
    ORMInt          ('imps_unverified',               'imps_unverified'),
    ORMInt          ('profiling_requests',            'profiling_requests'),
    ORMInt          ('requests',                      'requests'),
  ]

userrole = Object('userrole', 'Userrole', False)
userrole.id = PQIndex([ORMInt('user_role_id', 'user_role_id')], 'userrole_user_role_id_seq')
userrole.fields = [
    ORMInt          ('account_role_id',               'account_role_id', 'accountrole'),
    ORMInt          ('flags',                         'flags'),
    ORMString       ('internal_access_type',          'internal_access_type'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('ldap_dn',                       'ldap_dn'),
    ORMString       ('name',                          'name'),
    ORMTimestamp    ('version',                       'version'),
  ]

userroleinternalaccess = Object('userroleinternalaccess', 'Userroleinternalaccess', False)
userroleinternalaccess.id = PQIndex([
    ORMInt          ('user_role_id',                  'user_role_id'),
    ORMInt          ('account_id',                    'account_id'),
  ] )
userroleinternalaccess.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

users = Object('users', 'Users', False)
users.id = PQIndex([ORMInt('user_id', 'user_id')], 'users_user_id_seq')
users.fields = [
    ORMInt          ('account_id',                    'account_id', 'account'),
    ORMString       ('auth_type',                     'auth_type'),
    ORMString       ('email',                         'email'),
    ORMString       ('first_name',                    'first_name'),
    ORMInt          ('flags',                         'flags'),
    ORMString       ('job_title',                     'job_title'),
    ORMString       ('language',                      'language'),
    ORMString       ('last_name',                     'last_name'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('ldap_dn',                       'ldap_dn'),
    ORMString       ('phone',                         'phone'),
    ORMFloat        ('prepaid_amount_limit',          'prepaid_amount_limit'),
    ORMString       ('status',                        'status'),
    ORMInt          ('user_credential_id',            'user_credential_id'),
    ORMInt          ('user_role_id',                  'user_role_id', 'userrole'),
    ORMTimestamp    ('version',                       'version'),
  ]

usersite = Object('usersite', 'Usersite', False)
usersite.id = PQIndex([
    ORMInt          ('user_id',                       'user_id'),
    ORMInt          ('site_id',                       'site_id'),
  ] )
usersite.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

videostatsdaily = Object('videostatsdaily', 'Videostatsdaily', False)
videostatsdaily.id = PQIndex([
    ORMDate         ('adv_sdate',                     'adv_sdate'),
    ORMInt          ('cc_id',                         'cc_id'),
  ] )
videostatsdaily.fields = [
    ORMInt          ('adv_month',                     'adv_month'),
    ORMInt          ('adv_year',                      'adv_year'),
    ORMInt          ('ccg_id',                        'ccg_id'),
    ORMInt          ('complete',                      'complete'),
    ORMInt          ('mid',                           'mid'),
    ORMInt          ('mute',                          'mute'),
    ORMInt          ('ostart',                        'ostart'),
    ORMInt          ('oview',                         'oview'),
    ORMInt          ('pause',                         'pause'),
    ORMInt          ('q1',                            'q1'),
    ORMInt          ('q3',                            'q3'),
    ORMInt          ('skip',                          'skip'),
    ORMInt          ('unmute',                        'unmute'),
  ]

walledgarden = Object('walledgarden', 'Walledgarden', False)
walledgarden.id = PQIndex([ORMInt('wg_id', 'wg_id')], 'walledgarden_wg_id_seq')
walledgarden.fields = [
    ORMInt          ('agency_account_id',             'agency_account_id'),
    ORMString       ('agency_marketplace',            'agency_marketplace'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMInt          ('pub_account_id',                'pub_account_id'),
    ORMString       ('pub_marketplace',               'pub_marketplace'),
    ORMTimestamp    ('version',                       'version'),
  ]

wdrequestmapping = Object('wdrequestmapping', 'Wdrequestmapping', True)
wdrequestmapping.id = PQIndex([ORMInt('wd_req_mapping_id', 'wd_req_mapping_id')], 'wdrequestmapping_wd_req_mapping_id_seq')
wdrequestmapping.fields = [
    ORMString       ('description',                   'description'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('request',                       'request'),
    ORMTimestamp    ('version',                       'version'),
  ]

wdtag = Object('wdtag', 'WDTag', True)
wdtag.id = PQIndex([ORMInt('wdtag_id', 'wdtag_id')], 'wdtag_wdtag_id_seq')
wdtag.fields = [
    ORMFloat        ('height',                        'height'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('name',                          'name'),
    ORMString       ('opted_in_content',              'opted_in_content'),
    ORMString       ('opted_out_content',             'opted_out_content'),
    ORMString       ('passback',                      'passback'),
    ORMInt          ('site_id',                       'site_id', 'site'),
    ORMString       ('status',                        'status'),
    ORMInt          ('template_id',                   'template_id', 'template'),
    ORMTimestamp    ('version',                       'version', default = "timestamp 'now'"),
    ORMFloat        ('width',                         'width'),
  ]

wdtagfeed_optedin = Object('wdtagfeed_optedin', 'WDTagfeed_optedin', True)
wdtagfeed_optedin.id = PQIndex([
    ORMInt          ('wdtag_id',                      'wdtag_id'),
    ORMInt          ('feed_id',                       'feed_id'),
  ] )
wdtagfeed_optedin.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

wdtagfeed_optedout = Object('wdtagfeed_optedout', 'WDTagfeed_optedout', True)
wdtagfeed_optedout.id = PQIndex([
    ORMInt          ('wdtag_id',                      'wdtag_id'),
    ORMInt          ('feed_id',                       'feed_id'),
  ] )
wdtagfeed_optedout.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
  ]

wdtagoptgroupstate = Object('wdtagoptgroupstate', 'Wdtagoptgroupstate', False)
wdtagoptgroupstate.id = PQIndex([
    ORMInt          ('option_group_id',               'option_group_id'),
    ORMInt          ('wdtag_id',                      'wdtag_id'),
  ] )
wdtagoptgroupstate.fields = [
    ORMBool         ('collapsed',                     'collapsed'),
    ORMBool         ('enabled',                       'enabled'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMTimestamp    ('version',                       'version'),
  ]

wdtagoptionvalue = Object('wdtagoptionvalue', 'Wdtagoptionvalue', False)
wdtagoptionvalue.id = PQIndex([
    ORMInt          ('option_id',                     'option_id'),
    ORMInt          ('wdtag_id',                      'wdtag_id'),
  ] )
wdtagoptionvalue.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('value',                         'value'),
    ORMTimestamp    ('version',                       'version'),
  ]

webapplication = Object('webapplication', 'Webapplication', False)
webapplication.id = PQIndex([ORMString('name', 'name')])
webapplication.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('status',                        'status'),
  ]

webapplicationoperation = Object('webapplicationoperation', 'Webapplicationoperation', False)
webapplicationoperation.id = PQIndex([ORMString('name', 'name')])
webapplicationoperation.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('status',                        'status'),
  ]

webapplicationsource = Object('webapplicationsource', 'Webapplicationsource', False)
webapplicationsource.id = PQIndex([ORMString('name', 'name')])
webapplicationsource.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('status',                        'status'),
  ]

webbrowser = DualObject('webbrowser', 'Webbrowser', False)
webbrowser.id = PQIndex([
  ] )
webbrowser.fields = [
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('marker',                        'marker'),
    ORMString       ('name',                          'name'),
    ORMInt          ('priority',                      'priority'),
    ORMString       ('regexp',                        'regexp'),
    ORMString       ('regexp_required',               'regexp_required'),
  ]

weboperation = Object('weboperation', 'Weboperation', False)
weboperation.id = PQIndex([ORMInt('web_operation_id', 'web_operation_id')], 'weboperation_web_operation_id_seq')
weboperation.fields = [
    ORMString       ('app',                           'app'),
    ORMInt          ('flags',                         'flags'),
    ORMTimestamp    ('last_updated',                  'last_updated'),
    ORMString       ('operation',                     'operation'),
    ORMString       ('source',                        'source'),
    ORMString       ('status',                        'status'),
  ]

webstats = Object('webstats', 'Webstats', False)
webstats.id = PQIndex([
    ORMTimestamp    ('stimestamp',                    'stimestamp'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('ct',                            'ct'),
    ORMString       ('curct',                         'curct'),
    ORMInt          ('browser_property_id',           'browser_property_id'),
    ORMInt          ('os_property_id',                'os_property_id'),
    ORMInt          ('web_operation_id',              'web_operation_id'),
    ORMString       ('result',                        'result'),
    ORMString       ('user_status',                   'user_status'),
    ORMBool         ('test',                          'test'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('cc_id',                         'cc_id'),
  ] )
webstats.fields = [
    ORMInt          ('count',                         'count'),
    ORMInt          ('country_hour',                  'country_hour'),
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
  ]

webstatsdailycountry = Object('webstatsdailycountry', 'Webstatsdailycountry', False)
webstatsdailycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('web_operation_id',              'web_operation_id'),
    ORMString       ('result',                        'result'),
    ORMString       ('user_status',                   'user_status'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
webstatsdailycountry.fields = [
    ORMInt          ('count',                         'count'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
  ]

webstatsmonthlycountry = Object('webstatsmonthlycountry', 'Webstatsmonthlycountry', False)
webstatsmonthlycountry.id = PQIndex([
    ORMDate         ('country_sdate',                 'country_sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('web_operation_id',              'web_operation_id'),
    ORMString       ('result',                        'result'),
    ORMString       ('user_status',                   'user_status'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
webstatsmonthlycountry.fields = [
    ORMInt          ('count',                         'count'),
    ORMInt          ('country_smonth',                'country_smonth'),
    ORMInt          ('country_syear',                 'country_syear'),
  ]

webstatstotal = Object('webstatstotal', 'Webstatstotal', False)
webstatstotal.id = PQIndex([
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('web_operation_id',              'web_operation_id'),
    ORMString       ('result',                        'result'),
    ORMString       ('user_status',                   'user_status'),
    ORMInt          ('tag_id',                        'tag_id'),
    ORMInt          ('ccg_id',                        'ccg_id'),
  ] )
webstatstotal.fields = [
    ORMInt          ('count',                         'count'),
  ]

webwisediscoveritem = Object('webwisediscoveritem', 'WebwiseDiscoverItem', True)
webwisediscoveritem.id = PQIndex([ORMString('item_id', 'item_id')])
webwisediscoveritem.fields = [
    ORMString       ('language',                      'language'),
    ORMString       ('link',                          'link'),
    ORMTimestamp    ('pub_date',                      'pub_date'),
    ORMString       ('title',                         'title'),
  ]

webwisediscoveritemstats = Object('webwisediscoveritemstats', 'Webwisediscoveritemstats', False)
webwisediscoveritemstats.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMString       ('item_id',                       'item_id'),
    ORMString       ('xslt',                          'xslt'),
    ORMFloat        ('position',                      'position'),
    ORMString       ('random',                        'random'),
    ORMInt          ('wdtag_id',                      'wdtag_id'),
    ORMString       ('test',                          'test'),
    ORMString       ('user_status',                   'user_status'),
  ] )
webwisediscoveritemstats.fields = [
    ORMFloat        ('clicks',                        'clicks'),
    ORMFloat        ('impressions',                   'impressions'),
  ]

webwisediscovertagstats = Object('webwisediscovertagstats', 'Webwisediscovertagstats', False)
webwisediscovertagstats.id = PQIndex([
    ORMTimestamp    ('sdate',                         'sdate'),
    ORMInt          ('colo_id',                       'colo_id'),
    ORMInt          ('wdtag_id',                      'wdtag_id'),
    ORMString       ('opted_in',                      'opted_in'),
    ORMString       ('test',                          'test'),
  ] )
webwisediscovertagstats.fields = [
    ORMFloat        ('clicks',                        'clicks'),
    ORMFloat        ('impressions',                   'impressions'),
  ]

objects = [
    account,
    accountaddress,
    accountfinancialdata,
    accountfinancialsettings,
    accountrole,
    accounttype,
    accounttypecreativesize,
    accounttypecreativetemplate,
    accounttypedevicechannel,
    accounttype_ccgtype,
    action,
    actionrequests,
    actionrequestsdaily,
    actionrequestsdaily_country_action_date_y2014m10,
    actionrequestsdaily_country_action_date_y2015m04,
    actionrequestsmonthly,
    actionrequestsmonthly_country_action_date_y2014m10,
    actionrequestsmonthly_country_action_date_y2015m04,
    actionrequeststotal,
    actionrequests_country_action_date_y2014w43,
    actionrequests_country_action_date_y2015w16,
    actionrequests_country_action_date_y2015w17,
    actionstats,
    actiontype,
    adsconfig,
    advertiserdevicestatsdaily,
    advertiserstatsdaily,
    advertiserstatsdailycountry,
    advertiserstatsmonthly,
    advertiserstatstotal,
    advertiseruserstats,
    advertiseruserstatsrunning,
    advertiseruserstatstotal,
    appformat,
    auctionsettings,
    auditlog,
    authenticationtoken,
    behavioralparameters,
    behavioralparameterslist,
    birtreport,
    birtreportinstance,
    birtreportsession,
    campaign,
    campaignactionstatsdaily,
    campaignactionstatstotal,
    campaignallocation,
    campaigncoloactionstatsdaily,
    campaigncolotagactionstatsdaily,
    campaigncreative,
    campaigncreativegroup,
    campaigncredit,
    campaigncreditallocation,
    campaigncreditallocationusage,
    campaignexcludedchannel,
    campaignschedule,
    campaignstatsdaily,
    campaignstatsdailycountry,
    campaigntagactionstatsdaily,
    campaignuserstats,
    campaignuserstatsrunning,
    campaignuserstatstotal,
    ccauctionstatsdaily,
    ccgaction,
    ccgactionstatsdaily,
    ccgactionstatsdailyadvertiser,
    ccgactionstatstotal,
    ccgaroverride,
    ccgauctionstatsdaily,
    ccgauctionstatstotal,
    ccgcoloactionstatsdaily,
    ccgcoloauctionstatsdaily,
    ccgcolocation,
    ccgcolotagactionstatsdaily,
    ccgcriteriacombination,
    ccgcriterion,
    ccgctroverride,
    ccgdevicechannel,
    ccgdevicestatsdaily,
    ccggeochannel,
    ccgkeyword,
    ccgkeywordctroverride,
    ccgkeywordstatsdaily,
    ccgkeywordstatshourly,
    ccgkeywordstatstotal,
    ccgkeywordstatstow,
    ccgmobileopchannel,
    ccgrate,
    ccgschedule,
    ccgselectionfailure,
    ccgsite,
    ccgstatsdaily,
    ccgstatsdailycountry,
    ccgstatstotal,
    ccgtagactionstatsdaily,
    ccguserstats,
    ccguserstatsrunning,
    ccguserstatstotal,
    ccuserstats,
    ccuserstatsrunning,
    ccuserstatstotal,
    change_password_uid,
    channel,
    channelcategory,
    channelcountstats,
    channelimpinventory,
    channelinventory,
    channelinventorybycpm,
    channelinventorybycpmmonthly,
    channelinventorybycpmmonthlydates,
    channelinventoryestimstats,
    channeloverlapuserstats,
    channelrate,
    channelstats,
    channeltrigger,
    channeltriggerstats,
    channeltriggerstatstotal,
    channeltrigger_pcc_aa_channel_type_a,
    channeltrigger_pcc_aa_channel_type_d,
    channeltrigger_pcc_aa_channel_type_s,
    channeltrigger_pcc_cn_channel_type_a,
    channeltrigger_pcc_cn_channel_type_d,
    channeltrigger_pcc_cn_channel_type_s,
    channeltrigger_pcc_ru_channel_type_a,
    channeltrigger_pcc_ru_channel_type_d,
    channeltrigger_pcc_ru_channel_type_s,
    channelusagestatshourly,
    channelusagestatstotal,
    clobparams,
    cmprequeststatshourly,
    cmpstatsdaily,
    colocation,
    colocationrate,
    colorequeststats,
    colostats,
    colouserstats,
    colouserstatsrunning,
    colouserstatstotal,
    contentcategory,
    conversioncategory,
    conversionstatsdaily,
    conversionstatstotal,
    country,
    countryaddressfield,
    createduserstats,
    creative,
    creativecategory,
    creativecategorytype,
    creativecategory_account,
    creativecategory_creative,
    creativecategory_template,
    creativeoptgroupstate,
    creativeoptionvalue,
    creativerejectreason,
    creativesize,
    creativesizeexpansion,
    creative_tagsize,
    creative_tagsizetype,
    ctralgadvertiserexclusion,
    ctralgcampaignexclusion,
    ctralgorithm,
    currency,
    currencyexchange,
    currencyexchangerate,
    devicechannelcountstats,
    discoverchannelstate,
    displaystatus,
    dynamicresources,
    expressionperformance,
    expressionusedchannel,
    feed,
    feedstate,
    fraudcondition,
    freqcap,
    globalcolouserstats,
    historyctrstatshourly,
    historyctrstatstotal,
    historyctrstatstow,
    iasstatshourly,
    insertionorder,
    ispcampaignstatsdaily,
    ispstatsdaily,
    ispstatsdailycountry,
    ispstatstotal,
    keywordinventory,
    nbostats,
    objecttype,
    foros_applied_patches,
    foros_timed_services,
    optionenumvalue,
    optionfiletype,
    optiongroup,
    options,
    optoutstats,
    oracle_job,
    pageloadsdaily,
    passbackstats,
    placementblacklist,
    plan_log,
    platform,
    platformdetector,
    policy,
    publisherinventory,
    publisherstatsdaily,
    publisherstatsdailycountry,
    publisherstatstotal,
    requeststatsdailycountry,
    requeststatsdailyisp,
    requeststatshourly,
    requeststatsmonthlycountry,
    rtbcategory,
    rtbconnector,
    searchengine,
    searchenginestatsdaily,
    searchtermstatsdaily,
    site,
    sitecategory,
    sitechannelstats,
    sitecreativeapproval,
    sitecreativecategoryexclusion,
    siterate,
    siteuserstats,
    siteuserstatsrunning,
    siteuserstatstotal,
    sizetype,
    tagauctionsettings,
    tagauctionstats,
    tagcontentcategory,
    tagctroverride,
    tagoptgroupstate,
    tagoptionvalue,
    tagpositionstats,
    tagpricing,
    tags,
    tagscreativecategoryexclusion,
    tag_tagsize,
    template,
    templatefile,
    thirdpartycreative,
    timezone,
    top100urls,
    triggers,
    useradvertiser,
    useragentstats,
    userproperty,
    userpropertystatsdaily,
    userpropertystatshourly,
    userpropertystatsmonthly,
    userrole,
    userroleinternalaccess,
    users,
    usersite,
    videostatsdaily,
    walledgarden,
    wdrequestmapping,
    wdtag,
    wdtagfeed_optedin,
    wdtagfeed_optedout,
    wdtagoptgroupstate,
    wdtagoptionvalue,
    webapplication,
    webapplicationoperation,
    webapplicationsource,
    webbrowser,
    weboperation,
    webstats,
    webstatsdailycountry,
    webstatsmonthlycountry,
    webstatstotal,
    webwisediscoveritem,
    webwisediscoveritemstats,
    webwisediscovertagstats,
]

unimportant = [
    'SEARCHTERM',
    'USERCREDENTIALS',
    'APPLIED_SCN',
    'REPLICATION_HEARTBEAT',
    'REPLICATION_MARKER',
    'SCHEMA_APPLIED_PATCHES',
]
