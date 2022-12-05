from ORMBase import *

account = Object('ACCOUNT', 'Account', False)
account.id = Index([ORMInt('ACCOUNT_ID', 'id')], 'ACCOUNTSEQ')
account.fields = [
    ORMInt          ('ACCOUNT_MANAGER_ID',            'account_manager', 'USERS'),
    ORMInt          ('ACCOUNT_TYPE_ID',               'account_type', 'ACCOUNTTYPE'),
    ORMInt          ('ADV_CONTACT_ID',                'adv_contact', 'USERS'),
    ORMInt          ('AGENCY_ACCOUNT_ID',             'agency_account_id', 'ACCOUNT'),
    ORMInt          ('BILLING_ADDRESS_ID',            'billing_address_id', 'ACCOUNTADDRESS'),
    ORMString       ('BUSINESS_AREA',                 'business_area'),
    ORMInt          ('CMP_CONTACT_ID',                'cmp_contact_id', 'USERS'),
    ORMString       ('COMPANY_REGISTRATION_NUMBER',   'company_registration_number'),
    ORMString       ('CONTACT_NAME',                  'contact_name'),
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY', default = "'GN'"),
    ORMInt          ('CURRENCY_ID',                   'currency', 'CURRENCY'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('HID_PROFILE',                   'hid_profile'),
    ORMInt          ('INTERNAL_ACCOUNT_ID',           'internal_account', 'ACCOUNT'),
    ORMInt          ('ISP_CONTACT_ID',                'isp_contact', 'USERS'),
    ORMTimestamp    ('LAST_DEACTIVATED',              'last_deactivated'),
    ORMInt          ('LEGAL_ADDRESS_ID',              'legal_address_id', 'ACCOUNTADDRESS'),
    ORMString       ('LEGAL_NAME',                    'legal_name'),
    ORMInt          ('MESSAGE_SENT',                  'message_sent'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('NOTES',                         'notes'),
    ORMString       ('PASSBACK_BELOW_FOLD',           'passback_below_fold'),
    ORMInt          ('PUB_CONTACT_ID',                'pub_contact_id', 'USERS'),
    ORMString       ('PUB_PIXEL_OPTIN',               'pub_pixel_optin'),
    ORMString       ('PUB_PIXEL_OPTOUT',              'pub_pixel_optout'),
    ORMInt          ('ROLE_ID',                       'role', 'ACCOUNTROLE'),
    ORMString       ('SPECIFIC_BUSINESS_AREA',        'specific_business_area'),
    ORMString       ('STATUS',                        'status'),
    ORMString       ('TEXT_ADSERVING',                'text_adserving'),
    ORMInt          ('TIMEZONE_ID',                   'timezone_id', 'TIMEZONE'),
    ORMString       ('USE_PUB_PIXEL',                 'use_pub_pixel'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
account.asks = [
    Select('index', ['NAME', 'ROLE_ID' ]),
    Select('name', ['NAME']),
  ]
account.pgsync = True

accountaddress = Object('ACCOUNTADDRESS', 'AccountAddress', False)
accountaddress.id = Index([ORMInt('ADDRESS_ID', 'id')], 'ACCOUNTADDRESSSEQ')
accountaddress.fields = [
    ORMString       ('CITY',                          'city'),
    ORMString       ('LINE1',                         'line1'),
    ORMString       ('LINE2',                         'line2'),
    ORMString       ('LINE3',                         'line3'),
    ORMString       ('PROVINCE',                      'province'),
    ORMString       ('STATE',                         'state'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMString       ('ZIP',                           'zip'),
  ]
accountaddress.pgsync = True

accountfinancialdata = Object('ACCOUNTFINANCIALDATA', 'Accountfinancialdata', False)
accountfinancialdata.id = Index([ORMInt('ACCOUNT_ID', 'account_id')])
accountfinancialdata.fields = [
    ORMInt          ('CAMP_CREDIT_USED',              'camp_credit_used'),
    ORMInt          ('INVOICED_OUTSTANDING',          'invoiced_outstanding'),
    ORMInt          ('INVOICED_RECEIVED',             'invoiced_received'),
    ORMInt          ('NOT_INVOICED',                  'not_invoiced'),
    ORMInt          ('PAYMENTS_BILLED',               'payments_billed'),
    ORMInt          ('PAYMENTS_PAID',                 'payments_paid'),
    ORMInt          ('PAYMENTS_UNBILLED',             'payments_unbilled'),
    ORMInt          ('PREPAID_AMOUNT',                'prepaid_amount'),
    ORMInt          ('TOTAL_ADV_AMOUNT',              'total_adv_amount'),
    ORMInt          ('TOTAL_PAID',                    'total_paid'),
    ORMString       ('TYPE',                          'type'),
    ORMInt          ('UNBILLED_SCHEDULE_OF_WORKS',    'unbilled_schedule_of_works'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

accountfinancialdata_addb_2040 = DualObject('ACCOUNTFINANCIALDATA_ADDB_2040', 'Accountfinancialdata_addb_2040', False)
accountfinancialdata_addb_2040.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMString       ('BANK_ACCOUNT_CHECK_DIGITS',     'bank_account_check_digits'),
    ORMInt          ('BANK_ACCOUNT_CURRENCY_ID',      'bank_account_currency_id'),
    ORMString       ('BANK_ACCOUNT_IBAN',             'bank_account_iban'),
    ORMString       ('BANK_ACCOUNT_NAME',             'bank_account_name'),
    ORMString       ('BANK_ACCOUNT_NUMBER',           'bank_account_number'),
    ORMString       ('BANK_ACCOUNT_TYPE',             'bank_account_type'),
    ORMString       ('BANK_BIC_CODE',                 'bank_bic_code'),
    ORMString       ('BANK_BRANCH_NAME',              'bank_branch_name'),
    ORMString       ('BANK_COUNTRY_CODE',             'bank_country_code'),
    ORMString       ('BANK_NAME',                     'bank_name'),
    ORMString       ('BANK_NUMBER',                   'bank_number'),
    ORMString       ('BANK_SORT_CODE',                'bank_sort_code'),
    ORMString       ('BILLING_FREQUENCY',             'billing_frequency'),
    ORMInt          ('BILLING_FREQUENCY_OFFSET',      'billing_frequency_offset'),
    ORMFloat        ('COMMISSION',                    'commission'),
    ORMFloat        ('CREDIT_LIMIT',                  'credit_limit'),
    ORMInt          ('DEFAULT_BILL_TO_USER_ID',       'default_bill_to_user_id'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('INSURANCE_NUMBER',              'insurance_number'),
    ORMInt          ('INVOICED_OUTSTANDING',          'invoiced_outstanding'),
    ORMInt          ('INVOICED_RECEIVED',             'invoiced_received'),
    ORMString       ('IS_FROZEN',                     'is_frozen'),
    ORMInt          ('MIN_INVOICE',                   'min_invoice'),
    ORMInt          ('NOT_INVOICED',                  'not_invoiced'),
    ORMInt          ('ON_ACCOUNT_CREDIT',             'on_account_credit'),
    ORMDate         ('ON_ACCOUNT_CREDIT_UPDATED',     'on_account_credit_updated'),
    ORMInt          ('PAYMENTS_BILLED',               'payments_billed'),
    ORMInt          ('PAYMENTS_PAID',                 'payments_paid'),
    ORMInt          ('PAYMENTS_UNBILLED',             'payments_unbilled'),
    ORMString       ('PAYMENT_METHOD',                'payment_method'),
    ORMString       ('PAYMENT_TERMS',                 'payment_terms'),
    ORMString       ('TAX_NOTES',                     'tax_notes'),
    ORMString       ('TAX_NUMBER',                    'tax_number'),
    ORMFloat        ('TAX_RATE',                      'tax_rate'),
    ORMString       ('TYPE',                          'type'),
  ] )
accountfinancialdata_addb_2040.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

accountfinancialsettings = Object('ACCOUNTFINANCIALSETTINGS', 'Accountfinancialsettings', False)
accountfinancialsettings.id = Index([ORMInt('ACCOUNT_ID', 'account_id')])
accountfinancialsettings.fields = [
    ORMString       ('BANK_ACCOUNT_CHECK_DIGITS',     'bank_account_check_digits'),
    ORMInt          ('BANK_ACCOUNT_CURRENCY_ID',      'bank_account_currency_id', 'CURRENCY'),
    ORMString       ('BANK_ACCOUNT_IBAN',             'bank_account_iban'),
    ORMString       ('BANK_ACCOUNT_NAME',             'bank_account_name'),
    ORMString       ('BANK_ACCOUNT_NUMBER',           'bank_account_number'),
    ORMString       ('BANK_ACCOUNT_TYPE',             'bank_account_type'),
    ORMString       ('BANK_BIC_CODE',                 'bank_bic_code'),
    ORMString       ('BANK_BRANCH_NAME',              'bank_branch_name'),
    ORMString       ('BANK_COUNTRY_CODE',             'bank_country_code', 'COUNTRY'),
    ORMString       ('BANK_NAME',                     'bank_name'),
    ORMString       ('BANK_NUMBER',                   'bank_number'),
    ORMString       ('BANK_SORT_CODE',                'bank_sort_code'),
    ORMString       ('BILLING_FREQUENCY',             'billing_frequency'),
    ORMInt          ('BILLING_FREQUENCY_OFFSET',      'billing_frequency_offset'),
    ORMFloat        ('COMMISSION',                    'commission'),
    ORMFloat        ('CREDIT_LIMIT',                  'credit_limit'),
    ORMInt          ('DEFAULT_BILL_TO_USER_ID',       'default_bill_to_user_id', 'USERS'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('INSURANCE_NUMBER',              'insurance_number'),
    ORMString       ('IS_FROZEN',                     'is_frozen'),
    ORMInt          ('MIN_INVOICE',                   'min_invoice'),
    ORMDate         ('ON_ACCOUNT_CREDIT_UPDATED',     'on_account_credit_updated'),
    ORMString       ('PAYMENT_METHOD',                'payment_method'),
    ORMString       ('PAYMENT_TERMS',                 'payment_terms'),
    ORMString       ('TAX_NOTES',                     'tax_notes'),
    ORMString       ('TAX_NUMBER',                    'tax_number'),
    ORMFloat        ('TAX_RATE',                      'tax_rate'),
    ORMString       ('TYPE',                          'type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

accountrole = Object('ACCOUNTROLE', 'AccountRole', False)
accountrole.id = Index([ORMInt('ACCOUNT_ROLE_ID', 'id')])
accountrole.fields = [
    ORMString       ('NAME',                          'name'),
  ]
accountrole.pgsync = True

accounttype = Object('ACCOUNTTYPE', 'AccountType', False)
accounttype.id = Index([ORMInt('ACCOUNT_TYPE_ID', 'id')], 'ACCOUNTTYPESEQ')
accounttype.fields = [
    ORMInt          ('ACCOUNT_ROLE_ID',               'account_role', 'ACCOUNTROLE'),
    ORMString       ('ADV_EXCLUSIONS',                'adv_exclusions'),
    ORMString       ('ADV_EXCLUSION_APPROVAL',        'adv_exclusion_approval'),
    ORMString       ('AUCTION_RATE',                  'auction_rate'),
    ORMInt          ('CAMPAIGN_CHECK_1',              'campaign_check_1'),
    ORMInt          ('CAMPAIGN_CHECK_2',              'campaign_check_2'),
    ORMInt          ('CAMPAIGN_CHECK_3',              'campaign_check_3'),
    ORMString       ('CAMPAIGN_CHECK_ON',             'campaign_check_on'),
    ORMInt          ('CHANNEL_CHECK_1',               'channel_check_1'),
    ORMInt          ('CHANNEL_CHECK_2',               'channel_check_2'),
    ORMInt          ('CHANNEL_CHECK_3',               'channel_check_3'),
    ORMString       ('CHANNEL_CHECK_ON',              'channel_check_on'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('IO_MANAGEMENT',                 'io_management'),
    ORMInt          ('MAX_KEYWORDS_PER_CHANNEL',      'max_keywords_per_channel'),
    ORMInt          ('MAX_KEYWORDS_PER_GROUP',        'max_keywords_per_group'),
    ORMInt          ('MAX_KEYWORD_LENGTH',            'max_keyword_length'),
    ORMInt          ('MAX_URLS_PER_CHANNEL',          'max_urls_per_channel'),
    ORMInt          ('MAX_URL_LENGTH',                'max_url_length'),
    ORMInt          ('MOBILE_OPERATOR_TARGETING',     'mobile_operator_targeting'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('SHOW_BROWSER_PASSBACK_TAG',     'show_browser_passback_tag'),
    ORMString       ('SHOW_IFRAME_TAG',               'show_iframe_tag'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
accounttype.asks = [
    Select('name', ['NAME']),
  ]
accounttype.pgsync = True

accounttype_ccgtype = Object('ACCOUNTTYPE_CCGTYPE', 'Accounttype_ccgtype', False)
accounttype_ccgtype.id = Index([ORMInt('ACCOUNTTYPE_CCGTYPE_ID', 'accounttype_ccgtype_id')])
accounttype_ccgtype.fields = [
    ORMInt          ('ACCOUNT_TYPE_ID',               'account_type_id', 'ACCOUNTTYPE'),
    ORMString       ('CCG_TYPE',                      'ccg_type'),
    ORMString       ('RATE_TYPE',                     'rate_type'),
    ORMString       ('TGT_TYPE',                      'tgt_type'),
  ]

accounttypecreativesize = Object('ACCOUNTTYPECREATIVESIZE', 'AccountTypeCreativeSize', False)
accounttypecreativesize.id = Index([
    ORMInt          ('ACCOUNT_TYPE_ID',               'account_type_id'),
    ORMInt          ('SIZE_ID',                       'size_id'),
  ] )

accounttypecreativetemplate = Object('ACCOUNTTYPECREATIVETEMPLATE', 'AccountTypeCreativeTemplate', False)
accounttypecreativetemplate.id = Index([
    ORMInt          ('ACCOUNT_TYPE_ID',               'account_type_id'),
    ORMInt          ('TEMPLATE_ID',                   'template_id'),
  ] )

accounttypedevicechannel = Object('ACCOUNTTYPEDEVICECHANNEL', 'Accounttypedevicechannel', False)
accounttypedevicechannel.id = Index([
    ORMInt          ('ACCOUNT_TYPE_ID',               'account_type_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
  ] )

action = Object('ACTION', 'Action', False)
action.id = Index([ORMInt('ACTION_ID', 'action_id')], 'ACTIONSEQ')
action.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account_id', 'ACCOUNT'),
    ORMInt          ('CLICK_WINDOW',                  'click_window'),
    ORMInt          ('CONV_CATEGORY_ID',              'conv_category_id', default = "0"),
    ORMFloat        ('CUR_VALUE',                     'cur_value'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id'),
    ORMInt          ('IMP_WINDOW',                    'imp_window'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('STATS_STATUS',                  'stats_status'),
    ORMString       ('STATUS',                        'status'),
    ORMString       ('URL',                           'url'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
action.pgsync = True

actiontype = Object('ACTIONTYPE', 'ActionType', False)
actiontype.id = Index([ORMInt('ACTION_TYPE_ID', 'id')])
actiontype.fields = [
    ORMString       ('NAME',                          'name'),
  ]

adsconfig = Object('ADSCONFIG', 'AdsConfig', False)
adsconfig.id = Index([ORMString('PARAM_NAME', 'param_name')])
adsconfig.fields = [
    ORMString       ('PARAM_VALUE',                   'value'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

advertiserstats = DualObject('ADVERTISERSTATS', 'Advertiserstats', False)
advertiserstats.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_AMOUNT_CMP',                'adv_amount_cmp'),
    ORMInt          ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_CAMP_CREDITED_AMOUNT',      'adv_camp_credited_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMInt          ('ADV_CREDITED_ACTIONS',          'adv_credited_actions'),
    ORMInt          ('ADV_CREDITED_CLICKS',           'adv_credited_clicks'),
    ORMInt          ('ADV_CREDITED_IMPS',             'adv_credited_imps'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMString       ('HID_PROFILE',                   'hid_profile'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('REQUESTS',                      'requests'),
  ] )

advertiserstatsdaily = DualObject('ADVERTISERSTATSDAILY', 'Advertiserstatsdaily', False)
advertiserstatsdaily.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_AMOUNT_CMP',                'adv_amount_cmp'),
    ORMInt          ('ADV_CAMP_CREDITED_AMOUNT',      'adv_camp_credited_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_CREDITED_ACTIONS',          'adv_credited_actions'),
    ORMInt          ('ADV_CREDITED_CLICKS',           'adv_credited_clicks'),
    ORMInt          ('ADV_CREDITED_IMPS',             'adv_credited_imps'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMString       ('HID_PROFILE',                   'hid_profile'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('PUB_AMOUNT_ADV',                'pub_amount_adv'),
    ORMInt          ('PUB_COMM_AMOUNT_ADV',           'pub_comm_amount_adv'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMInt          ('SIZE_ID',                       'size_id'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

advertiseruserstats = Object('ADVERTISERUSERSTATS', 'Advertiseruserstats', False)
advertiseruserstats.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMDate         ('LAST_APPEARANCE_DATE',          'last_appearance_date'),
  ] )
advertiseruserstats.fields = [
    ORMInt          ('DISPLAY_UNIQUE_USERS',          'display_unique_users'),
    ORMInt          ('TEXT_UNIQUE_USERS',             'text_unique_users'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

advertiseruserstatsrunning = Object('ADVERTISERUSERSTATSRUNNING', 'Advertiseruserstatsrunning', False)
advertiseruserstatsrunning.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
  ] )
advertiseruserstatsrunning.fields = [
    ORMInt          ('DISPLAY_UNIQUE_USERS',          'display_unique_users'),
    ORMDate         ('LAST_ACTIVE_DATE',              'last_active_date'),
    ORMInt          ('TEXT_UNIQUE_USERS',             'text_unique_users'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

advertiseruserstatstotal = Object('ADVERTISERUSERSTATSTOTAL', 'Advertiseruserstatstotal', False)
advertiseruserstatstotal.id = Index([ORMInt('ADV_ACCOUNT_ID', 'adv_account_id')])
advertiseruserstatstotal.fields = [
    ORMInt          ('DISPLAY_UNIQUE_USERS',          'display_unique_users'),
    ORMInt          ('TEXT_UNIQUE_USERS',             'text_unique_users'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

advpubstatsdaily = Object('ADVPUBSTATSDAILY', 'Advpubstatsdaily', False)
advpubstatsdaily.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
  ] )
advpubstatsdaily.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
  ]

advpubstatstotal = Object('ADVPUBSTATSTOTAL', 'Advpubstatstotal', False)
advpubstatstotal.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
  ] )
advpubstatstotal.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
  ]

appformat = Object('APPFORMAT', 'AppFormat', False)
appformat.id = Index([ORMInt('APP_FORMAT_ID', 'id')], 'APPFORMATSEQ')
appformat.fields = [
    ORMString       ('MIME_TYPE',                     'mime_type'),
    ORMString       ('NAME',                          'name'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
appformat.asks = [
    Select('name', ['NAME']),
  ]
appformat.pgsync = True

auctionsettings = Object('AUCTIONSETTINGS', 'Auctionsettings', False)
auctionsettings.id = Index([ORMInt('ACCOUNT_ID', 'account_id')])
auctionsettings.fields = [
    ORMFloat        ('MAX_ECPM_SHARE',                'max_ecpm_share'),
    ORMInt          ('MAX_RANDOM_CPM',                'max_random_cpm'),
    ORMFloat        ('PROP_PROBABILITY_SHARE',        'prop_probability_share'),
    ORMFloat        ('RANDOM_SHARE',                  'random_share'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

authenticationtoken = Object('AUTHENTICATIONTOKEN', 'Authenticationtoken', False)
authenticationtoken.id = Index([ORMString('TOKEN', 'token')])
authenticationtoken.fields = [
    ORMString       ('IP',                            'ip'),
    ORMInt          ('LAST_UPDATE',                   'last_update'),
    ORMInt          ('USER_ID',                       'user_id'),
  ]

bash_tmp = DualObject('BASH_TMP', 'Bash_tmp', False)
bash_tmp.id = Index([
    ORMInt          ('A',                             'a'),
    ORMInt          ('B',                             'b'),
  ] )

behavioralparameters = Object('BEHAVIORALPARAMETERS', 'BehavioralParameters', False)
behavioralparameters.id = Index([ORMInt('BEHAV_PARAMS_ID', 'id')], 'BEHAVIORALPARAMETERSSEQ')
behavioralparameters.fields = [
    ORMInt          ('BEHAV_PARAMS_LIST_ID',          'behav_params_list_id', 'BEHAVIORALPARAMETERSLIST'),
    ORMInt          ('CHANNEL_ID',                    'channel', 'CHANNEL'),
    ORMInt          ('MINIMUM_VISITS',                'minimum_visits'),
    ORMInt          ('TIME_FROM',                     'time_from'),
    ORMInt          ('TIME_TO',                       'time_to'),
    ORMString       ('TRIGGER_TYPE',                  'trigger_type'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMInt          ('WEIGHT',                        'weight'),
  ]
behavioralparameters.asks = [
    Select('channel', ['CHANNEL_ID']),
  ]
behavioralparameters.pgsync = True

behavioralparameterslist = Object('BEHAVIORALPARAMETERSLIST', 'Behavioralparameterslist', False)
behavioralparameterslist.id = Index([ORMInt('BEHAV_PARAMS_LIST_ID', 'id')], 'BEHAVIORALPARAMSLISTSEQ')
behavioralparameterslist.fields = [
    ORMString       ('NAME',                          'name'),
    ORMInt          ('THRESHOLD',                     'threshold'),
    ORMTimestamp    ('VERSION',                       'version', default = "systimestamp"),
  ]
behavioralparameterslist.pgsync = True

birtreport = Object('BIRTREPORT', 'Birtreport', False)
birtreport.id = Index([ORMInt('BIRT_REPORT_ID', 'birt_report_id')], 'BIRTREPORTSEQ')
birtreport.fields = [
    ORMInt          ('BIRT_REPORT_TYPE_ID',           'birt_report_type_id'),
    ORMInt          ('INSTANCE_CACHE_TIME',           'instance_cache_time'),
    ORMString       ('NAME',                          'name'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

birtreportinstance = Object('BIRTREPORTINSTANCE', 'Birtreportinstance', False)
birtreportinstance.id = Index([ORMInt('BIRT_REPORT_INSTANCE_ID', 'birt_report_instance_id')], 'BIRTREPORTINSTANCESEQ')
birtreportinstance.fields = [
    ORMInt          ('BIRT_REPORT_ID',                'birt_report_id'),
    ORMTimestamp    ('CREATED_TIMESTAMP',             'created_timestamp'),
    ORMString       ('DOCUMENT_FILE_NAME',            'document_file_name'),
    ORMString       ('PARAMETERS_HASH',               'parameters_hash'),
    ORMInt          ('STATE',                         'state'),
  ]

birtreportsession = Object('BIRTREPORTSESSION', 'Birtreportsession', False)
birtreportsession.id = Index([ORMInt('BIRT_REPORT_SESSION_ID', 'birt_report_session_id')], 'BIRTREPORTSESSIONSEQ')
birtreportsession.fields = [
    ORMInt          ('BIRT_REPORT_ID',                'birt_report_id'),
    ORMInt          ('BIRT_REPORT_INSTANCE_ID',       'birt_report_instance_id'),
    ORMTimestamp    ('CREATED_TIMESTAMP',             'created_timestamp'),
    ORMString       ('PARAMETERS_HASH',               'parameters_hash'),
    ORMString       ('SESSION_ID',                    'session_id'),
    ORMInt          ('STATE',                         'state'),
    ORMInt          ('USER_ID',                       'user_id'),
  ]

campaign = Object('CAMPAIGN', 'Campaign', False)
campaign.id = Index([ORMInt('CAMPAIGN_ID', 'id')], 'CAMPAIGNSEQ')
campaign.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMInt          ('BILL_TO_USER_ID',               'bill_to_user', 'USERS'),
    ORMFloat        ('BUDGET',                        'budget'),
    ORMFloat        ('BUDGET_MANUAL',                 'budget_manual'),
    ORMString       ('CAMPAIGN_TYPE',                 'campaign_type'),
    ORMFloat        ('COMMISSION',                    'commission'),
    ORMFloat        ('DAILY_BUDGET',                  'daily_budget'),
    ORMDate         ('DATE_END',                      'date_end'),
    ORMDate         ('DATE_START',                    'date_start'),
    ORMString       ('DELIVERY_PACING',               'delivery_pacing'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMInt          ('FLAGS',                         'flags'),
    ORMInt          ('FREQ_CAP_ID',                   'freq_cap', 'FREQCAP'),
    ORMTimestamp    ('LAST_DEACTIVATED',              'last_deactivated'),
    ORMString       ('MARKETPLACE',                   'marketplace'),
    ORMFloat        ('MAX_PUB_SHARE',                 'max_pub_share'),
    ORMString       ('NAME',                          'name'),
    ORMInt          ('SALES_MANAGER_ID',              'sales_manager_id', 'USERS'),
    ORMInt          ('SOLD_TO_USER_ID',               'sold_to_user', 'USERS'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
campaign.asks = [
    Select('name', ['NAME']),
  ]
campaign.pgsync = True

campaignallocation = Object('CAMPAIGNALLOCATION', 'Campaignallocation', False)
campaignallocation.id = Index([ORMInt('CAMPAIGN_ALLOCATION_ID', 'campaign_allocation_id')], 'CAMPAIGNALLOCATIONSEQ')
campaignallocation.fields = [
    ORMInt          ('AMOUNT',                        'amount'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id', 'CAMPAIGN'),
    ORMInt          ('IO_ID',                         'io_id', 'INSERTIONORDER'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

campaignallocationusage = Object('CAMPAIGNALLOCATIONUSAGE', 'Campaignallocationusage', False)
campaignallocationusage.id = Index([ORMInt('CAMPAIGN_ALLOCATION_ID', 'campaign_allocation_id')])
campaignallocationusage.fields = [
    ORMDate         ('END_DATE',                      'end_date'),
    ORMInt          ('POSITION',                      'position'),
    ORMDate         ('START_DATE',                    'start_date'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('UTILIZED_AMOUNT',               'utilized_amount'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

campaigncreative = Object('CAMPAIGNCREATIVE', 'CampaignCreative', False)
campaigncreative.id = Index([ORMInt('CC_ID', 'id')], 'CAMPAIGNCREATIVESEQ')
campaigncreative.fields = [
    ORMInt          ('CCG_ID',                        'ccg', 'CAMPAIGNCREATIVEGROUP'),
    ORMInt          ('CREATIVE_ID',                   'creative', 'CREATIVE'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMInt          ('FREQ_CAP_ID',                   'freq_cap', 'FREQCAP'),
    ORMTimestamp    ('LAST_DEACTIVATED',              'last_deactivated'),
    ORMInt          ('SET_NUMBER',                    'set_number'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMInt          ('WEIGHT',                        'weight'),
  ]
campaigncreative.pgsync = True

campaigncreativegroup = Object('CAMPAIGNCREATIVEGROUP', 'CampaignCreativeGroup', False)
campaigncreativegroup.id = Index([ORMInt('CCG_ID', 'id')], 'CAMPAIGNCREATIVEGROUPSEQ')
campaigncreativegroup.fields = [
    ORMFloat        ('BUDGET',                        'budget', default = "1000000"),
    ORMInt          ('CAMPAIGN_ID',                   'campaign', 'CAMPAIGN'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate', 'CCGRATE'),
    ORMString       ('CCG_TYPE',                      'ccg_type', default = "'D'"),
    ORMInt          ('CHANNEL_ID',                    'channel', 'CHANNEL'),
    ORMString       ('CHANNEL_TARGET',                'channel_target'),
    ORMInt          ('CHECK_INTERVAL_NUM',            'check_interval_num'),
    ORMString       ('CHECK_NOTES',                   'check_notes'),
    ORMInt          ('CHECK_USER_ID',                 'check_user_id', 'USERS'),
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY', default = "'GN'"),
    ORMInt          ('CTR_RESET_ID',                  'ctr_reset_id'),
    ORMDate         ('CUR_DATE',                      'cur_date'),
    ORMFloat        ('DAILY_BUDGET',                  'daily_budget'),
    ORMInt          ('DAILY_CLICKS',                  'daily_clicks'),
    ORMInt          ('DAILY_IMP',                     'daily_imp'),
    ORMDate         ('DATE_END',                      'date_end'),
    ORMDate         ('DATE_START',                    'date_start'),
    ORMString       ('DELIVERY_PACING',               'delivery_pacing'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMInt          ('FLAGS',                         'flags'),
    ORMInt          ('FREQ_CAP_ID',                   'freq_cap', 'FREQCAP'),
    ORMDate         ('LAST_CHECK_DATE',               'last_check_date'),
    ORMTimestamp    ('LAST_DEACTIVATED',              'last_deactivated'),
    ORMInt          ('MIN_UID_AGE',                   'min_uid_age'),
    ORMString       ('NAME',                          'name'),
    ORMDate         ('NEXT_CHECK_DATE',               'next_check_date'),
    ORMString       ('OPTIN_STATUS_TARGETING',        'optin_status_targeting'),
    ORMDate         ('QA_DATE',                       'qa_date'),
    ORMString       ('QA_DESCRIPTION',                'qa_description'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMInt          ('QA_USER_ID',                    'qa_user_id', 'USERS'),
    ORMInt          ('REALIZED_ACTIONS',              'realized_actions'),
    ORMInt          ('REALIZED_BUDGET',               'realized_budget'),
    ORMInt          ('REALIZED_CLICKS',               'realized_clicks'),
    ORMInt          ('REALIZED_IMP',                  'realized_imp'),
    ORMInt          ('ROTATION_CRITERIA',             'rotation_criteria'),
    ORMInt          ('SELECTED_MOBILE_OPERATORS',     'selected_mobile_operators'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TARGETING_CHANNEL_ID',          'targeting_channel_id', 'CHANNEL'),
    ORMString       ('TGT_TYPE',                      'tgt_type', default = "'C'"),
    ORMInt          ('TOTAL_REACH',                   'total_reach'),
    ORMInt          ('USER_SAMPLE_GROUP_END',         'user_sample_group_end'),
    ORMInt          ('USER_SAMPLE_GROUP_START',       'user_sample_group_start'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
campaigncreativegroup.asks = [
    Select('name', ['NAME']),
  ]
campaigncreativegroup.pgsync = True

campaigncredit = Object('CAMPAIGNCREDIT', 'Campaigncredit', False)
campaigncredit.id = Index([ORMInt('CAMPAIGN_CREDIT_ID', 'campaign_credit_id')], 'CAMPAIGNCREDITSEQ')
campaigncredit.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account_id', 'ACCOUNT'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id', 'ACCOUNT'),
    ORMInt          ('AMOUNT',                        'amount'),
    ORMString       ('DESCRIPTION',                   'description'),
    ORMString       ('PUB_PAYMENT_OPTION',            'pub_payment_option'),
    ORMString       ('PURPOSE',                       'purpose'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

campaigncreditallocation = Object('CAMPAIGNCREDITALLOCATION', 'Campaigncreditallocation', False)
campaigncreditallocation.id = Index([ORMInt('CAMP_CREDIT_ALLOC_ID', 'camp_credit_alloc_id')], 'CAMPAIGNCREDITALLOCATIONSEQ')
campaigncreditallocation.fields = [
    ORMInt          ('ALLOCATED_AMOUNT',              'allocated_amount'),
    ORMInt          ('CAMPAIGN_CREDIT_ID',            'campaign_credit_id', 'CAMPAIGNCREDIT'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id', 'CAMPAIGN'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

campaigncreditallocationusage = Object('CAMPAIGNCREDITALLOCATIONUSAGE', 'Campaigncreditallocationusage', False)
campaigncreditallocationusage.id = Index([ORMInt('CAMP_CREDIT_ALLOC_ID', 'camp_credit_alloc_id')])
campaigncreditallocationusage.fields = [
    ORMInt          ('USED_AMOUNT',                   'used_amount'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

campaigncreditstatsdaily = Object('CAMPAIGNCREDITSTATSDAILY', 'Campaigncreditstatsdaily', False)
campaigncreditstatsdaily.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('AGENCY_ACCOUNT_ID',             'agency_account_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
  ] )
campaigncreditstatsdaily.fields = [
    ORMInt          ('ADV_CAMP_CREDITED_AMOUNT',      'adv_camp_credited_amount'),
  ]

campaignexcludedchannel = Object('CAMPAIGNEXCLUDEDCHANNEL', 'Campaignexcludedchannel', False)
campaignexcludedchannel.id = Index([
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
  ] )

campaignschedule = Object('CAMPAIGNSCHEDULE', 'Campaignschedule', False)
campaignschedule.id = Index([ORMInt('SCHEDULE_ID', 'schedule_id')], 'CAMPAIGNSCHEDULESEQ')
campaignschedule.fields = [
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id', 'CAMPAIGN'),
    ORMInt          ('TIME_FROM',                     'time_from'),
    ORMInt          ('TIME_TO',                       'time_to'),
  ]

campaignuserstats = Object('CAMPAIGNUSERSTATS', 'Campaignuserstats', False)
campaignuserstats.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('LAST_APPEARANCE_DATE',          'last_appearance_date'),
  ] )
campaignuserstats.fields = [
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

campaignuserstatsrunning = Object('CAMPAIGNUSERSTATSRUNNING', 'Campaignuserstatsrunning', False)
campaignuserstatsrunning.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
  ] )
campaignuserstatsrunning.fields = [
    ORMDate         ('LAST_ACTIVE_DATE',              'last_active_date'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

campaignuserstatstotal = Object('CAMPAIGNUSERSTATSTOTAL', 'Campaignuserstatstotal', False)
campaignuserstatstotal.id = Index([ORMInt('CAMPAIGN_ID', 'campaign_id')])
campaignuserstatstotal.fields = [
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

ccgaction = Object('CCGACTION', 'Ccgaction', False)
ccgaction.id = Index([
    ORMInt          ('ACTION_ID',                     'action_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
  ] )
ccgaction.pgsync = True

ccgaroverride = Object('CCGAROVERRIDE', 'Ccgaroverride', False)
ccgaroverride.id = Index([ORMInt('CCG_ID', 'ccg_id')])
ccgaroverride.fields = [
    ORMInt          ('AR',                            'ar'),
  ]

ccgcolocation = Object('CCGCOLOCATION', 'Ccgcolocation', False)
ccgcolocation.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
  ] )

ccgctroverride = Object('CCGCTROVERRIDE', 'Ccgctroverride', False)
ccgctroverride.id = Index([ORMInt('CCG_ID', 'ccg_id')])
ccgctroverride.fields = [
    ORMInt          ('CTR',                           'ctr'),
  ]

ccgdevicechannel = Object('CCGDEVICECHANNEL', 'Ccgdevicechannel', False)
ccgdevicechannel.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
  ] )

ccggeochannel = Object('CCGGEOCHANNEL', 'Ccggeochannel', False)
ccggeochannel.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('GEO_CHANNEL_ID',                'geo_channel_id'),
  ] )

ccghistoryctr = Object('CCGHISTORYCTR', 'Ccghistoryctr', False)
ccghistoryctr.id = Index([ORMInt('CCG_ID', 'ccg_id')])
ccghistoryctr.fields = [
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('IMPS',                          'imps'),
  ]

ccgkeyword = Object('CCGKEYWORD', 'CCGKeyword', False)
ccgkeyword.id = Index([ORMInt('CCG_KEYWORD_ID', 'id')], 'CCGKEYWORDSEQ')
ccgkeyword.fields = [
    ORMInt          ('CCG_ID',                        'ccg', 'CAMPAIGNCREATIVEGROUP'),
    ORMInt          ('CHANNEL_ID',                    'channel_id', 'CHANNEL'),
    ORMString       ('CLICK_URL',                     'click_url'),
    ORMFloat        ('MAX_CPC_BID',                   'max_cpc_bid'),
    ORMString       ('ORIGINAL_KEYWORD',              'original_keyword'),
    ORMString       ('STATUS',                        'status'),
    ORMString       ('TRIGGER_TYPE',                  'trigger_type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
ccgkeyword.pgsync = True

ccgkeywordctroverride = Object('CCGKEYWORDCTROVERRIDE', 'Ccgkeywordctroverride', False)
ccgkeywordctroverride.id = Index([ORMInt('CCG_KEYWORD_ID', 'ccg_keyword_id')])
ccgkeywordctroverride.fields = [
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('TOW',                           'tow'),
  ]

ccgkeywordhistoryctr = Object('CCGKEYWORDHISTORYCTR', 'Ccgkeywordhistoryctr', False)
ccgkeywordhistoryctr.id = Index([ORMInt('CCG_KEYWORD_ID', 'ccg_keyword_id')])
ccgkeywordhistoryctr.fields = [
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('TOW',                           'tow'),
  ]

ccgkeywordstatsdaily = Object('CCGKEYWORDSTATSDAILY', 'Ccgkeywordstatsdaily', False)
ccgkeywordstatsdaily.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CCG_KEYWORD_ID',                'ccg_keyword_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
  ] )
ccgkeywordstatsdaily.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('PUB_AMOUNT_ADV',                'pub_amount_adv'),
  ]

ccgkeywordstatshourly = Object('CCGKEYWORDSTATSHOURLY', 'Ccgkeywordstatshourly', False)
ccgkeywordstatshourly.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CCG_KEYWORD_ID',                'ccg_keyword_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMDate         ('SDATE',                         'sdate'),
  ] )
ccgkeywordstatshourly.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('PUB_AMOUNT_ADV',                'pub_amount_adv'),
  ]

ccgkeywordstatstotal = Object('CCGKEYWORDSTATSTOTAL', 'Ccgkeywordstatstotal', False)
ccgkeywordstatstotal.id = Index([
    ORMInt          ('CCG_KEYWORD_ID',                'ccg_keyword_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
  ] )
ccgkeywordstatstotal.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('PUB_AMOUNT_ADV',                'pub_amount_adv'),
  ]

ccgkeywordstatstow = Object('CCGKEYWORDSTATSTOW', 'Ccgkeywordstatstow', False)
ccgkeywordstatstow.id = Index([
    ORMInt          ('CCG_KEYWORD_ID',                'ccg_keyword_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('DAY_OF_WEEK',                   'day_of_week'),
    ORMInt          ('HOUR',                          'hour'),
  ] )
ccgkeywordstatstow.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('PUB_AMOUNT_ADV',                'pub_amount_adv'),
  ]

ccgmobileopchannel = Object('CCGMOBILEOPCHANNEL', 'Ccgmobileopchannel', False)
ccgmobileopchannel.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('MOBILE_OP_CHANNEL_ID',          'mobile_op_channel_id'),
  ] )

ccgrate = Object('CCGRATE', 'CCGRate', False)
ccgrate.id = Index([ORMInt('CCG_RATE_ID', 'id')], 'CCGRATESEQ')
ccgrate.fields = [
    ORMInt          ('CCG_ID',                        'ccg', 'CAMPAIGNCREATIVEGROUP'),
    ORMFloat        ('CPA',                           'cpa'),
    ORMFloat        ('CPC',                           'cpc'),
    ORMFloat        ('CPM',                           'cpm'),
    ORMDate         ('EFFECTIVE_DATE',                'effective_date', default = "SYSDATE"),
    ORMString       ('RATE_TYPE',                     'rate_type'),
  ]
ccgrate.pgsync = True

ccgschedule = Object('CCGSCHEDULE', 'Ccgschedule', False)
ccgschedule.id = Index([ORMInt('SCHEDULE_ID', 'schedule_id')], 'CCGSCHEDULESEQ')
ccgschedule.fields = [
    ORMInt          ('CCG_ID',                        'ccg_id', 'CAMPAIGNCREATIVEGROUP'),
    ORMInt          ('TIME_FROM',                     'time_from'),
    ORMInt          ('TIME_TO',                       'time_to'),
  ]

ccgsite = Object('CCGSITE', 'CCGSite', False)
ccgsite.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('SITE_ID',                       'site_id'),
  ] )
ccgsite.pgsync = True

ccgtaghourlystats = Object('CCGTAGHOURLYSTATS', 'Ccgtaghourlystats', False)
ccgtaghourlystats.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
ccgtaghourlystats.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_AMOUNT_CMP',                'adv_amount_cmp'),
    ORMInt          ('ADV_INVOICE_COMM_AMOUNT',       'adv_invoice_comm_amount'),
    ORMInt          ('POT_ADV_AMOUNT',                'pot_adv_amount'),
    ORMInt          ('POT_ADV_AMOUNT_CMP',            'pot_adv_amount_cmp'),
    ORMInt          ('POT_ADV_INVOICE_COMM_AMOUNT',   'pot_adv_invoice_comm_amount'),
    ORMInt          ('POT_ISP_AMOUNT_ADV',            'pot_isp_amount_adv'),
    ORMInt          ('POT_PUB_AMOUNT_ADV',            'pot_pub_amount_adv'),
  ]

ccguserstats = Object('CCGUSERSTATS', 'Ccguserstats', False)
ccguserstats.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('LAST_APPEARANCE_DATE',          'last_appearance_date'),
  ] )
ccguserstats.fields = [
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

ccguserstatsrunning = Object('CCGUSERSTATSRUNNING', 'Ccguserstatsrunning', False)
ccguserstatsrunning.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
  ] )
ccguserstatsrunning.fields = [
    ORMDate         ('LAST_ACTIVE_DATE',              'last_active_date'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

ccguserstatstotal = Object('CCGUSERSTATSTOTAL', 'Ccguserstatstotal', False)
ccguserstatstotal.id = Index([ORMInt('CCG_ID', 'ccg_id')])
ccguserstatstotal.fields = [
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

ccuserstats = Object('CCUSERSTATS', 'Ccuserstats', False)
ccuserstats.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('LAST_APPEARANCE_DATE',          'last_appearance_date'),
  ] )
ccuserstats.fields = [
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

ccuserstatsrunning = Object('CCUSERSTATSRUNNING', 'Ccuserstatsrunning', False)
ccuserstatsrunning.id = Index([
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CC_ID',                         'cc_id'),
  ] )
ccuserstatsrunning.fields = [
    ORMDate         ('LAST_ACTIVE_DATE',              'last_active_date'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

ccuserstatstotal = Object('CCUSERSTATSTOTAL', 'Ccuserstatstotal', False)
ccuserstatstotal.id = Index([ORMInt('CC_ID', 'cc_id')])
ccuserstatstotal.fields = [
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

changedobject = DualObject('CHANGEDOBJECT', 'Changedobject', False)
changedobject.id = Index([
    ORMInt          ('OBJECT_ID',                     'object_id'),
    ORMString       ('OBJECT_TYPE',                   'object_type'),
  ] )

channel = Object('CHANNEL', 'Channel', False)
channel.id = Index([ORMInt('CHANNEL_ID', 'id')], 'CHANNELSEQ')
channel.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMString       ('BASE_KEYWORD',                  'base_keyword'),
    ORMInt          ('BEHAV_PARAMS_LIST_ID',          'behav_params_list_id', 'BEHAVIORALPARAMETERSLIST'),
    ORMInt          ('CHANNEL_LIST_ID',               'channel_list_id'),
    ORMString       ('CHANNEL_NAME_MACRO',            'channel_name_macro'),
    ORMInt          ('CHANNEL_RATE_ID',               'channel_rate_id', 'CHANNELRATE'),
    ORMString       ('CHANNEL_TYPE',                  'type'),
    ORMInt          ('CHECK_INTERVAL_NUM',            'check_interval_num'),
    ORMString       ('CHECK_NOTES',                   'check_notes'),
    ORMInt          ('CHECK_USER_ID',                 'check_user_id', 'USERS'),
    ORMString       ('CITY_LIST',                     'city_list'),
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY', default = "'GN'"),
    ORMTimestamp    ('CREATED_DATE',                  'created_date'),
    ORMString       ('DESCRIPTION',                   'description'),
    ORMString       ('DISCOVER_ANNOTATION',           'discover_annotation'),
    ORMString       ('DISCOVER_QUERY',                'discover_query'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMInt          ('DISTINCT_URL_TRIGGERS_COUNT',   'distinct_url_triggers_count'),
    ORMString       ('EXPRESSION',                    'expression'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMInt          ('FREQ_CAP_ID',                   'freq_cap_id', 'FREQCAP'),
    ORMString       ('GEO_TYPE',                      'geo_type'),
    ORMString       ('KEYWORD_TRIGGER_MACRO',         'keyword_trigger_macro'),
    ORMString       ('LANGUAGE',                      'language'),
    ORMDate         ('LAST_CHECK_DATE',               'last_check_date'),
    ORMFloat        ('LATITUDE',                      'latitude'),
    ORMFloat        ('LONGITUDE',                     'longitude'),
    ORMInt          ('MESSAGE_SENT',                  'message_sent'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('NAMESPACE',                     'channel_namespace'),
    ORMString       ('NEWSGATE_CATEGORY_NAME',        'newsgate_category_name'),
    ORMDate         ('NEXT_CHECK_DATE',               'next_check_date'),
    ORMInt          ('PARENT_CHANNEL_ID',             'parent_channel_id', 'CHANNEL'),
    ORMDate         ('QA_DATE',                       'qa_date'),
    ORMString       ('QA_DESCRIPTION',                'qa_description'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMInt          ('QA_USER_ID',                    'qa_user_id', 'USERS'),
    ORMInt          ('SIZE_ID',                       'size_id'),
    ORMString       ('STATUS',                        'status'),
    ORMDate         ('STATUS_CHANGE_DATE',            'status_change_date', default = "systimestamp"),
    ORMInt          ('SUPERSEDED_BY_CHANNEL_ID',      'superseded_by_channel_id', 'CHANNEL'),
    ORMString       ('TRIGGERS_STATUS',               'triggers_status'),
    ORMTimestamp    ('TRIGGERS_VERSION',              'triggers_version'),
    ORMString       ('TRIGGER_TYPE',                  'trigger_type'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMString       ('VISIBILITY',                    'visibility'),
  ]
channel.asks = [
    Select('name', ['NAME']),
  ]
channel.pgsync = True

channelcategory = Object('CHANNELCATEGORY', 'Channelcategory', False)
channelcategory.id = Index([
    ORMInt          ('CATEGORY_CHANNEL_ID',           'category_channel_id'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
  ] )

channelimpinventory = Object('CHANNELIMPINVENTORY', 'Channelimpinventory', False)
channelimpinventory.id = Index([
    ORMString       ('CCG_TYPE',                      'ccg_type'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('SDATE',                         'sdate'),
  ] )
channelimpinventory.fields = [
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPOPS_NO_IMP',                 'impops_no_imp'),
    ORMInt          ('IMPOPS_NO_IMP_USER_COUNT',      'impops_no_imp_user_count'),
    ORMInt          ('IMPOPS_NO_IMP_VALUE',           'impops_no_imp_value'),
    ORMInt          ('IMPOPS_USER_COUNT',             'impops_user_count'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_OTHER',                    'imps_other'),
    ORMInt          ('IMPS_OTHER_USER_COUNT',         'imps_other_user_count'),
    ORMInt          ('IMPS_OTHER_VALUE',              'imps_other_value'),
    ORMInt          ('IMPS_USER_COUNT',               'imps_user_count'),
    ORMInt          ('IMPS_VALUE',                    'imps_value'),
    ORMInt          ('REVENUE',                       'revenue'),
  ]

channelinventory = Object('CHANNELINVENTORY', 'ChannelInventory', False)
channelinventory.id = Index([
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('SDATE',                         'sdate'),
  ] )
channelinventory.fields = [
    ORMInt          ('ACTIVE_USER_COUNT',             'active_user_count'),
    ORMInt          ('HITS',                          'hits'),
    ORMInt          ('HITS_KWS',                      'hits_kws'),
    ORMInt          ('HITS_REPEAT_KWS',               'hits_repeat_kws'),
    ORMInt          ('HITS_SEARCH_KWS',               'hits_search_kws'),
    ORMInt          ('HITS_URLS',                     'hits_urls'),
    ORMFloat        ('SUM_ECPM',                      'sum_ecpm'),
    ORMInt          ('TOTAL_USER_COUNT',              'total_user_count'),
  ]

channelrate = Object('CHANNELRATE', 'Channelrate', False)
channelrate.id = Index([ORMInt('CHANNEL_RATE_ID', 'channel_rate_id')], 'CHANNELRATESEQ')
channelrate.fields = [
    ORMInt          ('CHANNEL_ID',                    'channel_id', 'CHANNEL'),
    ORMFloat        ('CPC',                           'cpc'),
    ORMFloat        ('CPM',                           'cpm'),
    ORMInt          ('CURRENCY_ID',                   'currency_id', 'CURRENCY'),
    ORMDate         ('EFFECTIVE_DATE',                'effective_date'),
    ORMString       ('RATE_TYPE',                     'rate_type'),
  ]
channelrate.pgsync = True

check_ = DualObject('CHECK_', 'Check_', False)
check_.id = Index([ORMInt('CHECK_ID', 'check_id')])

cmprequeststatshourly = Object('CMPREQUESTSTATSHOURLY', 'Cmprequeststatshourly', False)
cmprequeststatshourly.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMInt          ('CHANNEL_RATE_ID',               'channel_rate_id'),
    ORMInt          ('CMP_ACCOUNT_ID',                'cmp_account_id'),
    ORMDate         ('CMP_SDATE',                     'cmp_sdate'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )
cmprequeststatshourly.fields = [
    ORMFloat        ('ADV_AMOUNT_CMP',                'adv_amount_cmp'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMFloat        ('CMP_AMOUNT',                    'cmp_amount'),
    ORMFloat        ('CMP_AMOUNT_GLOBAL',             'cmp_amount_global'),
    ORMInt          ('IMPS',                          'imps'),
  ]

colocation = Object('COLOCATION', 'Colocation', False)
colocation.id = Index([ORMInt('COLO_ID', 'id')], 'COLOCATIONSEQ')
colocation.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate', 'COLOCATIONRATE'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('OPTOUT_SERVING',                'optout_serving'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
colocation.asks = [
    Select('name', ['NAME']),
  ]
colocation.pgsync = True

colocationrate = Object('COLOCATIONRATE', 'ColocationRate', False)
colocationrate.id = Index([ORMInt('COLO_RATE_ID', 'id')], 'COLOCATIONRATESEQ')
colocationrate.fields = [
    ORMInt          ('COLO_ID',                       'colo', 'COLOCATION'),
    ORMDate         ('EFFECTIVE_DATE',                'effective_date', default = "SYSDATE"),
    ORMFloat        ('REVENUE_SHARE',                 'revenue_share'),
  ]
colocationrate.pgsync = True

colousers = Object('COLOUSERS', 'ColoUsers', False)
colousers.id = Index([
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('CREATED',                       'created'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
  ] )
colousers.fields = [
    ORMInt          ('DAILY_NETWORK_USERS',           'daily_network_users'),
    ORMInt          ('DAILY_USERS',                   'daily_users'),
    ORMInt          ('MONTHLY_NETWORK_USERS',         'monthly_network_users'),
    ORMInt          ('MONTHLY_USERS',                 'monthly_users'),
    ORMInt          ('WEEKLY_USERS',                  'weekly_users'),
  ]

colouserstats = Object('COLOUSERSTATS', 'Colouserstats', False)
colouserstats.id = Index([
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('CREATE_DATE',                   'create_date'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMDate         ('LAST_APPEARANCE_DATE',          'last_appearance_date'),
  ] )
colouserstats.fields = [
    ORMInt          ('NETWORK_UNIQUE_USERS',          'network_unique_users'),
    ORMInt          ('PROFILING_UNIQUE_USERS',        'profiling_unique_users'),
    ORMInt          ('UNIQUE_HIDS',                   'unique_hids'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

contentcategory = Object('CONTENTCATEGORY', 'Contentcategory', False)
contentcategory.id = Index([ORMInt('CONTENT_CATEGORY_ID', 'content_category_id')], 'CONTENTCATEGORYSEQ')
contentcategory.fields = [
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY'),
    ORMString       ('NAME',                          'name'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

conversioncategory = Object('CONVERSIONCATEGORY', 'Conversioncategory', False)
conversioncategory.id = Index([ORMInt('CONV_CATEGORY_ID', 'conv_category_id')])
conversioncategory.fields = [
    ORMString       ('NAME',                          'name'),
  ]

country = Object('COUNTRY', 'Country', False)
country.id = Index([ORMString('COUNTRY_CODE', 'country_code')])
country.fields = [
    ORMString       ('ADSERVING_DOMAIN',              'adserving_domain'),
    ORMString       ('AD_FOOTER_URL',                 'ad_footer_url'),
    ORMString       ('AD_TAG_DOMAIN',                 'ad_tag_domain'),
    ORMString       ('CONVERSION_TAG_DOMAIN',         'conversion_tag_domain'),
    ORMInt          ('COUNTRY_ID',                    'country_id'),
    ORMInt          ('CURRENCY_ID',                   'currency', 'CURRENCY'),
    ORMFloat        ('DEFAULT_AGENCY_COMMISSION',     'default_agency_commission'),
    ORMInt          ('DEFAULT_PAYMENT_TERMS',         'default_payment_terms'),
    ORMFloat        ('DEFAULT_VAT_RATE',              'default_vat_rate'),
    ORMString       ('DISCOVER_DOMAIN',               'discover_domain'),
    ORMInt          ('HIGH_CHANNEL_THRESHOLD',        'high_channel_threshold'),
    ORMInt          ('INVOICE_CUSTOM_REPORT_ID',      'invoice_custom_report_id'),
    ORMString       ('LANGUAGE',                      'language'),
    ORMInt          ('LOW_CHANNEL_THRESHOLD',         'low_channel_threshold'),
    ORMFloat        ('MAX_URL_TRIGGER_SHARE',         'max_url_trigger_share'),
    ORMInt          ('MIN_TAG_VISIBILITY',            'min_tag_visibility'),
    ORMInt          ('MIN_URL_TRIGGER_THRESHOLD',     'min_url_trigger_threshold'),
    ORMInt          ('SORTORDER',                     'sortorder'),
    ORMString       ('STATIC_DOMAIN',                 'static_domain'),
    ORMInt          ('TIMEZONE_ID',                   'timezone_id', 'TIMEZONE'),
    ORMString       ('VAT_ENABLED',                   'vat_enabled'),
    ORMString       ('VAT_NUMBER_INPUT_ENABLED',      'vat_number_input_enabled'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
country.pgsync = True


countryaddressfield = Object('COUNTRYADDRESSFIELD', 'Countryaddressfield', False)
countryaddressfield.id = Index([ORMInt('COUNTRY_ADDRESS_FIELD_ID', 'country_address_field_id')], 'COUNTRYADDRESSFIELDSEQ')
countryaddressfield.fields = [
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY'),
    ORMString       ('FIELD_NAME',                    'field_name'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('NAME',                          'name'),
    ORMInt          ('ORDER_NUMBER',                  'order_number'),
    ORMString       ('RESOURCE_KEY',                  'resource_key'),
  ]

creative = Object('CREATIVE', 'Creative', False)
creative.id = Index([ORMInt('CREATIVE_ID', 'id')], 'CREATIVESEQ')
creative.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMString       ('EXPANDABLE',                    'expandable'),
    ORMString       ('EXPANSION',                     'expansion'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMTimestamp    ('LAST_DEACTIVATED',              'last_deactivated'),
    ORMString       ('NAME',                          'name'),
    ORMDate         ('QA_DATE',                       'qa_date'),
    ORMString       ('QA_DESCRIPTION',                'qa_description'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMInt          ('QA_USER_ID',                    'qa_user_id', 'USERS'),
    ORMInt          ('SIZE_ID',                       'size', 'CREATIVESIZE'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TEMPLATE_ID',                   'template_id', 'TEMPLATE'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
creative.asks = [
    Select('name', ['NAME']),
  ]
creative.pgsync = True

creative_tagsize = Object('CREATIVE_TAGSIZE', 'Creative_tagsize', False)
creative_tagsize.id = Index([
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('SIZE_ID',                       'size_id'),
  ] )
creative_tagsize.pgsync = True

creative_tagsizetype = Object('CREATIVE_TAGSIZETYPE', 'Creative_tagsizetype', False)
creative_tagsizetype.id = Index([
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('SIZE_TYPE_ID',                  'size_type_id'),
  ] )

creativecategory = Object('CREATIVECATEGORY', 'CreativeCategory', False)
creativecategory.id = Index([ORMInt('CREATIVE_CATEGORY_ID', 'id')], 'CREATIVECATEGORYSEQ')
creativecategory.fields = [
    ORMInt          ('CCT_ID',                        'type', 'CREATIVECATEGORYTYPE'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
creativecategory.asks = [
    Select('index', ['NAME', 'CCT_ID' ]),
    Select('name', ['NAME']),
  ]
creativecategory.pgsync = True

creativecategory_creative = Object('CREATIVECATEGORY_CREATIVE', 'CreativeCategory_Creative', False)
creativecategory_creative.id = Index([
    ORMInt          ('CREATIVE_CATEGORY_ID',          'creative_category_id'),
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
  ] )
creativecategory_creative.pgsync = True

creativecategory_template = Object('CREATIVECATEGORY_TEMPLATE', 'Creativecategory_template', False)
creativecategory_template.id = Index([
    ORMInt          ('CREATIVE_CATEGORY_ID',          'creative_category_id'),
    ORMInt          ('TEMPLATE_ID',                   'template_id'),
  ] )

creativecategorytype = Object('CREATIVECATEGORYTYPE', 'CreativeCategoryType', False)
creativecategorytype.id = Index([ORMInt('CCT_ID', 'id')])
creativecategorytype.fields = [
    ORMString       ('NAME',                          'name'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
creativecategorytype.asks = [
    Select('name', ['NAME']),
  ]
creativecategorytype.pgsync = True

creativeoptgroupstate = Object('CREATIVEOPTGROUPSTATE', 'Creativeoptgroupstate', False)
creativeoptgroupstate.id = Index([
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('OPTION_GROUP_ID',               'option_group_id'),
  ] )
creativeoptgroupstate.fields = [
    ORMString       ('COLLAPSED',                     'collapsed'),
    ORMString       ('ENABLED',                       'enabled'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

creativeoptionvalue = Object('CREATIVEOPTIONVALUE', 'CreativeOptionValue', False)
creativeoptionvalue.id = Index([
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('OPTION_ID',                     'option_id'),
  ] )
creativeoptionvalue.fields = [
    ORMString       ('VALUE',                         'value'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
creativeoptionvalue.pgsync = True

creativerejectreason = Object('CREATIVEREJECTREASON', 'Creativerejectreason', False)
creativerejectreason.id = Index([ORMInt('REJECT_REASON_ID', 'reject_reason_id')])
creativerejectreason.fields = [
    ORMString       ('DESCRIPTION',                   'description'),
  ]
creativerejectreason.pgsync = True

creativesize = Object('CREATIVESIZE', 'CreativeSize', False)
creativesize.id = Index([ORMInt('SIZE_ID', 'id')], 'CREATIVESIZESEQ')
creativesize.fields = [
    ORMInt          ('FLAGS',                         'flags'),
    ORMInt          ('HEIGHT',                        'height'),
    ORMInt          ('MAX_HEIGHT',                    'max_height'),
    ORMInt          ('MAX_WIDTH',                     'max_width'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('PROTOCOL_NAME',                 'protocol_name'),
    ORMInt          ('SIZE_TYPE_ID',                  'size_type_id', 'SIZETYPE'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMInt          ('WIDTH',                         'width'),
  ]
creativesize.asks = [
    Select('index', ['NAME', 'PROTOCOL_NAME' ]),
    Select('name', ['NAME']),
  ]
creativesize.pgsync = True

creativesizeexpansion = Object('CREATIVESIZEEXPANSION', 'Creativesizeexpansion', False)
creativesizeexpansion.id = Index([
    ORMString       ('EXPANSION',                     'expansion'),
    ORMInt          ('SIZE_ID',                       'size_id'),
  ] )

ctr_chg_log = DualObject('CTR_CHG_LOG', 'Ctr_chg_log', False)
ctr_chg_log.id = Index([
    ORMInt          ('CLICKS',                        'clicks'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('ID',                            'id'),
    ORMInt          ('IMPS',                          'imps'),
    ORMString       ('LOG_TYPE',                      'log_type'),
  ] )

ctr_kw_tow_matrix = Object('CTR_KW_TOW_MATRIX', 'Ctr_kw_tow_matrix', False)
ctr_kw_tow_matrix.id = Index([
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('DAY_OF_WEEK',                   'day_of_week'),
    ORMInt          ('HOUR',                          'hour'),
    ORMInt          ('ID',                            'id'),
    ORMString       ('LOG_TYPE',                      'log_type'),
  ] )
ctr_kw_tow_matrix.fields = [
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('IMPS',                          'imps'),
  ]

ctr_kwtg_log = DualObject('CTR_KWTG_LOG', 'Ctr_kwtg_log', False)
ctr_kwtg_log.id = Index([
    ORMInt          ('CLICKS',                        'clicks'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('ID',                            'id'),
    ORMInt          ('IMPS',                          'imps'),
    ORMString       ('LOG_TYPE',                      'log_type'),
  ] )

ctr_pta_log = DualObject('CTR_PTA_LOG', 'Ctr_pta_log', False)
ctr_pta_log.id = Index([
    ORMInt          ('CLICKS',                        'clicks'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('ID',                            'id'),
    ORMInt          ('IMPS',                          'imps'),
    ORMString       ('LOG_TYPE',                      'log_type'),
  ] )

ctralgadvertiserexclusion = Object('CTRALGADVERTISEREXCLUSION', 'Ctralgadvertiserexclusion', False)
ctralgadvertiserexclusion.id = Index([
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
  ] )

ctralgcampaignexclusion = Object('CTRALGCAMPAIGNEXCLUSION', 'Ctralgcampaignexclusion', False)
ctralgcampaignexclusion.id = Index([
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
  ] )

ctralgorithm = Object('CTRALGORITHM', 'Ctralgorithm', False)
ctralgorithm.id = Index([ORMString('COUNTRY_CODE', 'country_code')])
ctralgorithm.fields = [
    ORMInt          ('CAMPAIGN_TOW_LEVEL',            'campaign_tow_level'),
    ORMInt          ('CCGKEYWORD_KW_CTR_LEVEL',       'ccgkeyword_kw_ctr_level'),
    ORMInt          ('CCGKEYWORD_KW_TOW_LEVEL',       'ccgkeyword_kw_tow_level'),
    ORMInt          ('CCGKEYWORD_TG_CTR_LEVEL',       'ccgkeyword_tg_ctr_level'),
    ORMInt          ('CCGKEYWORD_TG_TOW_LEVEL',       'ccgkeyword_tg_tow_level'),
    ORMInt          ('CLICKS_INTERVAL1_DAYS',         'clicks_interval1_days'),
    ORMFloat        ('CLICKS_INTERVAL1_WEIGHT',       'clicks_interval1_weight'),
    ORMInt          ('CLICKS_INTERVAL2_DAYS',         'clicks_interval2_days'),
    ORMFloat        ('CLICKS_INTERVAL2_WEIGHT',       'clicks_interval2_weight'),
    ORMFloat        ('CLICKS_INTERVAL3_WEIGHT',       'clicks_interval3_weight'),
    ORMInt          ('CPA_RANDOM_IMPS',               'cpa_random_imps'),
    ORMInt          ('CPC_RANDOM_IMPS',               'cpc_random_imps'),
    ORMInt          ('IMPS_INTERVAL1_DAYS',           'imps_interval1_days'),
    ORMFloat        ('IMPS_INTERVAL1_WEIGHT',         'imps_interval1_weight'),
    ORMInt          ('IMPS_INTERVAL2_DAYS',           'imps_interval2_days'),
    ORMFloat        ('IMPS_INTERVAL2_WEIGHT',         'imps_interval2_weight'),
    ORMFloat        ('IMPS_INTERVAL3_WEIGHT',         'imps_interval3_weight'),
    ORMInt          ('KEYWORD_CTR_LEVEL',             'keyword_ctr_level'),
    ORMInt          ('KEYWORD_TOW_LEVEL',             'keyword_tow_level'),
    ORMFloat        ('KWTG_CTR_DEFAULT',              'kwtg_ctr_default'),
    ORMFloat        ('PUB_CTR_DEFAULT',               'pub_ctr_default'),
    ORMInt          ('PUB_CTR_LEVEL',                 'pub_ctr_level'),
    ORMInt          ('SITE_CTR_LEVEL',                'site_ctr_level'),
    ORMInt          ('SYS_CTR_LEVEL',                 'sys_ctr_level'),
    ORMInt          ('SYS_KWTG_CTR_LEVEL',            'sys_kwtg_ctr_level'),
    ORMInt          ('SYS_TOW_LEVEL',                 'sys_tow_level'),
    ORMInt          ('TAG_CTR_LEVEL',                 'tag_ctr_level'),
    ORMInt          ('TG_TOW_LEVEL',                  'tg_tow_level'),
    ORMFloat        ('TOW_RAW',                       'tow_raw'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

ctrchggtt = DualObject('CTRCHGGTT', 'Ctrchggtt', False)
ctrchggtt.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('CAMPAIGN_STATUS',               'campaign_status'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMString       ('CCG_STATUS',                    'ccg_status'),
    ORMString       ('CHG_TYPE',                      'chg_type'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMDate         ('CLICKS_INTERVAL1_END',          'clicks_interval1_end'),
    ORMDate         ('CLICKS_INTERVAL1_START',        'clicks_interval1_start'),
    ORMDate         ('CLICKS_INTERVAL2_END',          'clicks_interval2_end'),
    ORMDate         ('CLICKS_INTERVAL2_START',        'clicks_interval2_start'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMDate         ('CTR_RESET_DATE_CAMPAIGN',       'ctr_reset_date_campaign'),
    ORMDate         ('CTR_RESET_DATE_CCG',            'ctr_reset_date_ccg'),
    ORMInt          ('IMPS',                          'imps'),
    ORMDate         ('IMPS_INTERVAL1_END',            'imps_interval1_end'),
    ORMDate         ('IMPS_INTERVAL1_START',          'imps_interval1_start'),
    ORMDate         ('IMPS_INTERVAL2_END',            'imps_interval2_end'),
    ORMDate         ('IMPS_INTERVAL2_START',          'imps_interval2_start'),
    ORMDate         ('SDATE',                         'sdate'),
  ] )

ctrhistory = DualObject('CTRHISTORY', 'Ctrhistory', False)
ctrhistory.id = Index([
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CLICKS_INTERVAL',               'clicks_interval'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMString       ('CTR_RESET_ID_MATCHES',          'ctr_reset_id_matches'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_INTERVAL',                 'imps_interval'),
    ORMString       ('TOW_MATCHES',                   'tow_matches'),
  ] )

ctrkwtggtt = DualObject('CTRKWTGGTT', 'Ctrkwtggtt', False)
ctrkwtggtt.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('CAMPAIGN_STATUS',               'campaign_status'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_KEYWORD_ID',                'ccg_keyword_id'),
    ORMString       ('CCG_KEYWORD_STATUS',            'ccg_keyword_status'),
    ORMString       ('CCG_STATUS',                    'ccg_status'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMString       ('CHANNEL_STATUS',                'channel_status'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CLICKS_INTERVAL',               'clicks_interval'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_INTERVAL',                 'imps_interval'),
  ] )

ctrkwtgtowgtt = DualObject('CTRKWTGTOWGTT', 'Ctrkwtgtowgtt', False)
ctrkwtgtowgtt.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('CAMPAIGN_STATUS',               'campaign_status'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_KEYWORD_ID',                'ccg_keyword_id'),
    ORMString       ('CCG_KEYWORD_STATUS',            'ccg_keyword_status'),
    ORMString       ('CCG_STATUS',                    'ccg_status'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMString       ('CHANNEL_STATUS',                'channel_status'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CLICKS_INTERVAL',               'clicks_interval'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_INTERVAL',                 'imps_interval'),
    ORMString       ('TOW_MATCHES',                   'tow_matches'),
  ] )

ctrptagtt = DualObject('CTRPTAGTT', 'Ctrptagtt', False)
ctrptagtt.id = Index([
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CLICKS_INTERVAL',               'clicks_interval'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_INTERVAL',                 'imps_interval'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMInt          ('SITE_ID',                       'site_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )

currency = Object('CURRENCY', 'Currency', False)
currency.id = Index([ORMInt('CURRENCY_ID', 'id')], 'CURRENCYSEQ')
currency.fields = [
    ORMString       ('CURRENCY_CODE',                 'code'),
    ORMInt          ('FRACTION_DIGITS',               'fraction_digits'),
    ORMString       ('SOURCE',                        'source'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
currency.asks = [
    Select('code', ['CURRENCY_CODE']),
  ]
currency.pgsync = True

currencyexchange = Object('CURRENCYEXCHANGE', 'CurrencyExchange', False)
currencyexchange.id = Index([ORMInt('CURRENCY_EXCHANGE_ID', 'id')], 'CURRENCYEXCHANGESEQ')
currencyexchange.fields = [
    ORMDate         ('EFFECTIVE_DATE',                'effective_date'),
  ]
currencyexchange.pgsync = True

currencyexchangerate = Object('CURRENCYEXCHANGERATE', 'CurrencyExchangeRate', False)
currencyexchangerate.id = Index([
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMInt          ('CURRENCY_ID',                   'currency_id'),
  ] )
currencyexchangerate.fields = [
    ORMDate         ('LAST_UPDATED_DATE',             'last_updated_date'),
    ORMFloat        ('RATE',                          'rate'),
  ]
currencyexchangerate.asks = [
    Select('currency', ['CURRENCY_ID']),
  ]
currencyexchangerate.pgsync = True

displaystatus = Object('DISPLAYSTATUS', 'Displaystatus', False)
displaystatus.id = Index([
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id'),
    ORMInt          ('OBJECT_TYPE_ID',                'object_type_id'),
  ] )
displaystatus.fields = [
    ORMString       ('ADSERVER_STATUS',               'adserver_status'),
    ORMString       ('DESCRIPTION',                   'description'),
    ORMString       ('DISP_STATUS',                   'disp_status'),
  ]

expressionusedchannel = Object('EXPRESSIONUSEDCHANNEL', 'Expressionusedchannel', False)
expressionusedchannel.id = Index([
    ORMInt          ('EXPRESSION_CHANNEL_ID',         'expression_channel_id'),
    ORMInt          ('USED_CHANNEL_ID',               'used_channel_id'),
  ] )

feed = Object('FEED', 'Feed', False)
feed.id = Index([ORMInt('FEED_ID', 'feed_id')], 'FEEDSEQ')
feed.fields = [
    ORMInt          ('ITEMS',                         'items'),
    ORMDate         ('LAST_UPDATE',                   'last_update'),
    ORMString       ('URL',                           'url'),
  ]
feed.pgsync = True

fraudcondition = Object('FRAUDCONDITION', 'Fraudcondition', False)
fraudcondition.id = Index([ORMInt('FRAUD_CONDITION_ID', 'fraud_condition_id')], 'FRAUDCONDITIONSEQ')
fraudcondition.fields = [
    ORMInt          ('LIMIT',                         'limit'),
    ORMInt          ('PERIOD',                        'period'),
    ORMString       ('TYPE',                          'type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

freqcap = Object('FREQCAP', 'FreqCap', False)
freqcap.id = Index([ORMInt('FREQ_CAP_ID', 'id')], 'FREQCAPSEQ')
freqcap.fields = [
    ORMInt          ('LIFE_COUNT',                    'life_count'),
    ORMInt          ('PERIOD',                        'period'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMInt          ('WINDOW_COUNT',                  'window_count'),
    ORMInt          ('WINDOW_LENGTH',                 'window_length'),
  ]
freqcap.pgsync = True

globalcolousers = Object('GLOBALCOLOUSERS', 'Globalcolousers', False)
globalcolousers.id = Index([
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('CREATED',                       'created'),
    ORMDate         ('SDATE',                         'sdate'),
  ] )
globalcolousers.fields = [
    ORMInt          ('DAILY_NETWORK_USERS',           'daily_network_users'),
    ORMInt          ('DAILY_USERS',                   'daily_users'),
    ORMInt          ('MONTHLY_NETWORK_USERS',         'monthly_network_users'),
    ORMInt          ('MONTHLY_USERS',                 'monthly_users'),
    ORMInt          ('WEEKLY_USERS',                  'weekly_users'),
  ]

globalcolouserstats = Object('GLOBALCOLOUSERSTATS', 'Globalcolouserstats', False)
globalcolouserstats.id = Index([
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('CREATE_DATE',                   'create_date'),
    ORMDate         ('GLOBAL_SDATE',                  'global_sdate'),
    ORMDate         ('LAST_APPEARANCE_DATE',          'last_appearance_date'),
  ] )
globalcolouserstats.fields = [
    ORMInt          ('NETWORK_UNIQUE_USERS',          'network_unique_users'),
    ORMInt          ('PROFILING_UNIQUE_USERS',        'profiling_unique_users'),
    ORMInt          ('UNIQUE_HIDS',                   'unique_hids'),
    ORMInt          ('UNIQUE_USERS',                  'unique_users'),
  ]

historyctrstatstotal = Object('HISTORYCTRSTATSTOTAL', 'Historyctrstatstotal', False)
historyctrstatstotal.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CTR_RESET_ID',                  'ctr_reset_id'),
  ] )
historyctrstatstotal.fields = [
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPS',                          'imps'),
  ]

insertionorder = Object('INSERTIONORDER', 'Insertionorder', False)
insertionorder.id = Index([ORMInt('IO_ID', 'io_id')], 'INSERTIONORDERSEQ')
insertionorder.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account_id', 'ACCOUNT'),
    ORMFloat        ('AMOUNT',                        'amount'),
    ORMString       ('IO_NUMBER',                     'io_number'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('NOTES',                         'notes'),
    ORMString       ('PO_NUMBER',                     'po_number'),
    ORMString       ('PROBABILITY',                   'probability'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoice = Object('INVOICE', 'Invoice', False)
invoice.id = Index([ORMInt('INVOICE_ID', 'id')], 'INVOICESEQ')
invoice.fields = [
    ORMString       ('ADVERTISER_NAME',               'advertiser_name'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMInt          ('BILL_TO_USER_ADDRESS_ID',       'bill_to_user_address', 'ACCOUNTADDRESS'),
    ORMString       ('BILL_TO_USER_EMAIL',            'bill_to_user_email'),
    ORMInt          ('BILL_TO_USER_ID',               'bill_to_user', 'USERS'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign', 'CAMPAIGN'),
    ORMString       ('CAMPAIGN_NAME',                 'campaign_name'),
    ORMDate         ('CLOSED_DATE',                   'closed_date'),
    ORMInt          ('CMP_AMOUNT_NET',                'cmp_amount_net'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('CREDIT_SETTLEMENT',             'credit_settlement'),
    ORMInt          ('DEDUCT_FROM_PREPAID_AMOUNT',    'deduct_from_prepaid_amount'),
    ORMDate         ('DUE_DATE',                      'due_date'),
    ORMDate         ('END_BILLING_PERIOD',            'end_billing_period'),
    ORMInt          ('INVOICE_COMM_AMOUNT',           'invoice_comm_amount'),
    ORMDate         ('INVOICE_DATE',                  'invoice_date'),
    ORMDate         ('INVOICE_EMAIL_DATE',            'invoice_email_date'),
    ORMString       ('INVOICE_LEGAL_NUMBER',          'invoice_legal_number'),
    ORMInt          ('LINE_COUNT',                    'line_count'),
    ORMInt          ('OPEN_AMOUNT',                   'open_amount'),
    ORMInt          ('OPEN_AMOUNT_NET',               'open_amount_net'),
    ORMInt          ('ORIGINAL_INVOICE_ID',           'original_invoice', 'INVOICE'),
    ORMInt          ('PAID_AMOUNT',                   'paid_amount'),
    ORMInt          ('PUB_AMOUNT_NET',                'pub_amount_net'),
    ORMInt          ('SOLD_TO_USER_ADDRESS_ID',       'sold_to_user_address', 'ACCOUNTADDRESS'),
    ORMString       ('SOLD_TO_USER_EMAIL',            'sold_to_user_email'),
    ORMInt          ('SOLD_TO_USER_ID',               'sold_to_user', 'USERS'),
    ORMDate         ('START_BILLING_PERIOD',          'start_billing_period'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TOTAL_AMOUNT',                  'total_amount'),
    ORMInt          ('TOTAL_AMOUNT_CREDITED',         'total_amount_credited'),
    ORMInt          ('TOTAL_AMOUNT_DUE',              'total_amount_due'),
    ORMInt          ('TOTAL_AMOUNT_NET',              'total_amount_net'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ]

invoice_addb1895 = DualObject('INVOICE_ADDB1895', 'Invoice_addb1895', False)
invoice_addb1895.id = Index([
    ORMInt          ('ADJUSTED_AMOUNT',               'adjusted_amount'),
    ORMString       ('ADVERTISER_NAME',               'advertiser_name'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMInt          ('BILL_TO_USER_ADDRESS_ID',       'bill_to_user_address_id'),
    ORMString       ('BILL_TO_USER_EMAIL',            'bill_to_user_email'),
    ORMInt          ('BILL_TO_USER_ID',               'bill_to_user_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('CAMPAIGN_NAME',                 'campaign_name'),
    ORMDate         ('CLOSED_DATE',                   'closed_date'),
    ORMInt          ('CMP_AMOUNT_NET',                'cmp_amount_net'),
    ORMInt          ('CMP_AMOUNT_NET_ROUNDED',        'cmp_amount_net_rounded'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('COMM_AMOUNT_ROUNDED',           'comm_amount_rounded'),
    ORMInt          ('CREDITED_AMOUNT',               'credited_amount'),
    ORMDate         ('DUE_DATE',                      'due_date'),
    ORMDate         ('END_BILLING_PERIOD',            'end_billing_period'),
    ORMDate         ('INVOICE_DATE',                  'invoice_date'),
    ORMDate         ('INVOICE_EMAIL_DATE',            'invoice_email_date'),
    ORMInt          ('INVOICE_ID',                    'invoice_id'),
    ORMInt          ('LINE_COUNT',                    'line_count'),
    ORMInt          ('OPEN_AMOUNT',                   'open_amount'),
    ORMInt          ('OPEN_AMOUNT_NET',               'open_amount_net'),
    ORMString       ('ORACLE_INVOICE_NUM',            'oracle_invoice_num'),
    ORMInt          ('ORIGINAL_INVOICE_ID',           'original_invoice_id'),
    ORMInt          ('PAID_AMOUNT',                   'paid_amount'),
    ORMString       ('PDF_URL',                       'pdf_url'),
    ORMInt          ('PUB_AMOUNT_NET',                'pub_amount_net'),
    ORMInt          ('PUB_AMOUNT_NET_ROUNDED',        'pub_amount_net_rounded'),
    ORMString       ('PURCHASE_ORDER',                'purchase_order'),
    ORMInt          ('SOLD_TO_USER_ADDRESS_ID',       'sold_to_user_address_id'),
    ORMString       ('SOLD_TO_USER_EMAIL',            'sold_to_user_email'),
    ORMInt          ('SOLD_TO_USER_ID',               'sold_to_user_id'),
    ORMDate         ('START_BILLING_PERIOD',          'start_billing_period'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TAX_AMOUNT',                    'tax_amount'),
    ORMInt          ('TAX_AMOUNT_ROUNDED',            'tax_amount_rounded'),
    ORMInt          ('TOTAL_AMOUNT',                  'total_amount'),
    ORMInt          ('TOTAL_AMOUNT_DUE',              'total_amount_due'),
    ORMInt          ('TOTAL_AMOUNT_NET',              'total_amount_net'),
    ORMInt          ('TOTAL_AMOUNT_NET_ROUNDED',      'total_amount_net_rounded'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )
invoice_addb1895.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoice_addb_2040 = DualObject('INVOICE_ADDB_2040', 'Invoice_addb_2040', False)
invoice_addb_2040.id = Index([
    ORMInt          ('ADJUSTED_AMOUNT',               'adjusted_amount'),
    ORMString       ('ADVERTISER_NAME',               'advertiser_name'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMInt          ('BILL_TO_USER_ADDRESS_ID',       'bill_to_user_address_id'),
    ORMString       ('BILL_TO_USER_EMAIL',            'bill_to_user_email'),
    ORMInt          ('BILL_TO_USER_ID',               'bill_to_user_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('CAMPAIGN_NAME',                 'campaign_name'),
    ORMDate         ('CLOSED_DATE',                   'closed_date'),
    ORMInt          ('CMP_AMOUNT_NET',                'cmp_amount_net'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('CREDITED_AMOUNT',               'credited_amount'),
    ORMDate         ('DUE_DATE',                      'due_date'),
    ORMDate         ('END_BILLING_PERIOD',            'end_billing_period'),
    ORMInt          ('INVOICE_COMM_AMOUNT',           'invoice_comm_amount'),
    ORMDate         ('INVOICE_DATE',                  'invoice_date'),
    ORMDate         ('INVOICE_EMAIL_DATE',            'invoice_email_date'),
    ORMInt          ('INVOICE_ID',                    'invoice_id'),
    ORMInt          ('LINE_COUNT',                    'line_count'),
    ORMInt          ('OPEN_AMOUNT',                   'open_amount'),
    ORMInt          ('OPEN_AMOUNT_NET',               'open_amount_net'),
    ORMString       ('ORACLE_INVOICE_NUM',            'oracle_invoice_num'),
    ORMInt          ('ORIGINAL_INVOICE_ID',           'original_invoice_id'),
    ORMInt          ('PAID_AMOUNT',                   'paid_amount'),
    ORMInt          ('PUB_AMOUNT_NET',                'pub_amount_net'),
    ORMString       ('PURCHASE_ORDER',                'purchase_order'),
    ORMInt          ('SOLD_TO_USER_ADDRESS_ID',       'sold_to_user_address_id'),
    ORMString       ('SOLD_TO_USER_EMAIL',            'sold_to_user_email'),
    ORMInt          ('SOLD_TO_USER_ID',               'sold_to_user_id'),
    ORMDate         ('START_BILLING_PERIOD',          'start_billing_period'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TAX_AMOUNT',                    'tax_amount'),
    ORMInt          ('TOTAL_AMOUNT',                  'total_amount'),
    ORMInt          ('TOTAL_AMOUNT_DUE',              'total_amount_due'),
    ORMInt          ('TOTAL_AMOUNT_NET',              'total_amount_net'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )
invoice_addb_2040.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicedata = Object('INVOICEDATA', 'InvoiceData', False)
invoicedata.id = Index([ORMInt('INVOICE_DATA_ID', 'id')], 'INVOICEDATASEQ')
invoicedata.fields = [
    ORMInt          ('CCG_ID',                        'ccg', 'CAMPAIGNCREATIVEGROUP'),
    ORMString       ('DESCRIPTION',                   'description'),
    ORMInt          ('INVOICE_ID',                    'invoice', 'INVOICE'),
    ORMString       ('ISCMP',                         'iscmp'),
    ORMInt          ('ORIGINAL_INVOICE_DATA_ID',      'original_invoice_data', 'INVOICEDATA'),
    ORMInt          ('QUANTITY',                      'quantity'),
    ORMInt          ('REPORTED_QUANTITY',             'reported_quantity'),
    ORMString       ('UNIT_OF_MEASURE',               'unit_of_measure'),
    ORMFloat        ('UNIT_PRICE',                    'unit_price'),
    ORMFloat        ('UNIT_RATE',                     'unit_rate'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicedata_addb1895 = DualObject('INVOICEDATA_ADDB1895', 'Invoicedata_addb1895', False)
invoicedata_addb1895.id = Index([
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMString       ('DESCRIPTION',                   'description'),
    ORMInt          ('INVOICE_DATA_ID',               'invoice_data_id'),
    ORMInt          ('INVOICE_ID',                    'invoice_id'),
    ORMString       ('ISCMP',                         'iscmp'),
    ORMInt          ('ORIGINAL_INVOICE_DATA_ID',      'original_invoice_data_id'),
    ORMInt          ('QUANTITY',                      'quantity'),
    ORMInt          ('REPORTED_QUANTITY',             'reported_quantity'),
    ORMString       ('UNIT_OF_MEASURE',               'unit_of_measure'),
    ORMFloat        ('UNIT_PRICE',                    'unit_price'),
  ] )
invoicedata_addb1895.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicedatadetail = Object('INVOICEDATADETAIL', 'InvoiceDataDetail', False)
invoicedatadetail.id = Index([ORMInt('INVOICE_DATA_DETAIL_ID', 'id')])
invoicedatadetail.fields = [
    ORMInt          ('AMOUNT_NET',                    'amount_net'),
    ORMInt          ('AMOUNT_NET_CREDITED',           'amount_net_credited'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('COMM_AMOUNT_CREDITED',          'comm_amount_credited'),
    ORMInt          ('INVOICE_COMM_AMOUNT',           'invoice_comm_amount'),
    ORMInt          ('INVOICE_COMM_AMOUNT_CREDITED',  'invoice_comm_amount_credited'),
    ORMInt          ('INVOICE_DATA_ID',               'invoice_data', 'INVOICEDATA'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account', 'ACCOUNT'),
    ORMInt          ('ISP_AMOUNT_NET',                'isp_amount_net'),
    ORMInt          ('ISP_AMOUNT_NET_CREDITED',       'isp_amount_net_credited'),
    ORMInt          ('ISP_AMOUNT_NET_ORIGINAL',       'isp_amount_net_original'),
    ORMInt          ('ISP_BILL_ID',                   'isp_bill', 'SELFBILL'),
    ORMInt          ('PAYABLE_ACCOUNT_ID',            'payable_account_id', 'ACCOUNT'),
    ORMInt          ('PAYABLE_AMNT_NET_ADV_CREDITED', 'payable_amnt_net_adv_credited'),
    ORMInt          ('PAYABLE_AMOUNT_NET',            'payable_amount_net'),
    ORMInt          ('PAYABLE_AMOUNT_NET_ADV',        'payable_amount_net_adv'),
    ORMInt          ('PAYABLE_AMOUNT_NET_CREDITED',   'payable_amount_net_credited'),
    ORMInt          ('PAYABLE_AMOUNT_NET_ORIGINAL',   'payable_amount_net_original'),
    ORMInt          ('PAYABLE_BILL_ID',               'payable_bill_id', 'SELFBILL'),
    ORMInt          ('PAYABLE_COMM_AMOUNT',           'payable_comm_amount'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_CREDITED',  'payable_comm_amount_credited'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_ORIGINAL',  'payable_comm_amount_original'),
    ORMString       ('PWP',                           'pwp'),
    ORMInt          ('QUANTITY',                      'quantity'),
    ORMInt          ('QUANTITY_CREDITED',             'quantity_credited'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicedatadetail_addb1895 = DualObject('INVOICEDATADETAIL_ADDB1895', 'Invoicedatadetail_addb1895', False)
invoicedatadetail_addb1895.id = Index([
    ORMInt          ('AMOUNT_NET',                    'amount_net'),
    ORMInt          ('AMOUNT_NET_ROUNDED',            'amount_net_rounded'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('COMM_AMOUNT_ROUNDED',           'comm_amount_rounded'),
    ORMInt          ('INVOICE_DATA_DETAIL_ID',        'invoice_data_detail_id'),
    ORMInt          ('INVOICE_DATA_ID',               'invoice_data_id'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMInt          ('ISP_AMOUNT_NET',                'isp_amount_net'),
    ORMInt          ('ISP_AMOUNT_NET_ROUNDED',        'isp_amount_net_rounded'),
    ORMInt          ('ISP_BILL_ID',                   'isp_bill_id'),
    ORMInt          ('ISP_TAX_AMOUNT',                'isp_tax_amount'),
    ORMInt          ('ISP_TAX_AMOUNT_ROUNDED',        'isp_tax_amount_rounded'),
    ORMInt          ('PAYABLE_ACCOUNT_ID',            'payable_account_id'),
    ORMInt          ('PAYABLE_AMOUNT_NET',            'payable_amount_net'),
    ORMInt          ('PAYABLE_AMOUNT_NET_ROUNDED',    'payable_amount_net_rounded'),
    ORMInt          ('PAYABLE_BILL_ID',               'payable_bill_id'),
    ORMInt          ('PAYABLE_COMM_AMOUNT',           'payable_comm_amount'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_ROUNDED',   'payable_comm_amount_rounded'),
    ORMInt          ('PAYABLE_TAX_AMOUNT',            'payable_tax_amount'),
    ORMInt          ('PAYABLE_TAX_AMOUNT_ROUNDED',    'payable_tax_amount_rounded'),
    ORMString       ('PWP',                           'pwp'),
    ORMInt          ('QUANTITY',                      'quantity'),
  ] )
invoicedatadetail_addb1895.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicedatadetail_addb_2040 = DualObject('INVOICEDATADETAIL_ADDB_2040', 'Invoicedatadetail_addb_2040', False)
invoicedatadetail_addb_2040.id = Index([
    ORMInt          ('AMOUNT_NET',                    'amount_net'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('INVOICE_COMM_AMOUNT',           'invoice_comm_amount'),
    ORMInt          ('INVOICE_COMM_AMOUNT_ROUNDED',   'invoice_comm_amount_rounded'),
    ORMInt          ('INVOICE_DATA_DETAIL_ID',        'invoice_data_detail_id'),
    ORMInt          ('INVOICE_DATA_ID',               'invoice_data_id'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMInt          ('ISP_AMOUNT_NET',                'isp_amount_net'),
    ORMInt          ('ISP_BILL_ID',                   'isp_bill_id'),
    ORMInt          ('ISP_TAX_AMOUNT',                'isp_tax_amount'),
    ORMInt          ('PAYABLE_ACCOUNT_ID',            'payable_account_id'),
    ORMInt          ('PAYABLE_AMOUNT_NET',            'payable_amount_net'),
    ORMInt          ('PAYABLE_BILL_ID',               'payable_bill_id'),
    ORMInt          ('PAYABLE_COMM_AMOUNT',           'payable_comm_amount'),
    ORMInt          ('PAYABLE_TAX_AMOUNT',            'payable_tax_amount'),
    ORMString       ('PWP',                           'pwp'),
    ORMInt          ('QUANTITY',                      'quantity'),
  ] )
invoicedatadetail_addb_2040.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicedatadetail_addb_2144 = DualObject('INVOICEDATADETAIL_ADDB_2144', 'Invoicedatadetail_addb_2144', False)
invoicedatadetail_addb_2144.id = Index([
    ORMInt          ('AMOUNT_NET',                    'amount_net'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('INVOICE_COMM_AMOUNT',           'invoice_comm_amount'),
    ORMInt          ('INVOICE_DATA_DETAIL_ID',        'invoice_data_detail_id'),
    ORMInt          ('INVOICE_DATA_ID',               'invoice_data_id'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMInt          ('ISP_AMOUNT_NET',                'isp_amount_net'),
    ORMInt          ('ISP_BILL_ID',                   'isp_bill_id'),
    ORMInt          ('PAYABLE_ACCOUNT_ID',            'payable_account_id'),
    ORMInt          ('PAYABLE_AMOUNT_NET',            'payable_amount_net'),
    ORMInt          ('PAYABLE_BILL_ID',               'payable_bill_id'),
    ORMInt          ('PAYABLE_COMM_AMOUNT',           'payable_comm_amount'),
    ORMString       ('PWP',                           'pwp'),
    ORMInt          ('QUANTITY',                      'quantity'),
  ] )
invoicedatadetail_addb_2144.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

invoicehourly_addb_2144 = DualObject('INVOICEHOURLY_ADDB_2144', 'Invoicehourly_addb_2144', False)
invoicehourly_addb_2144.id = Index([
    ORMInt          ('ADV_AMOUNT_GLEDGER',            'adv_amount_gledger'),
    ORMInt          ('INVOICE_DATA_DETAIL_ID',        'invoice_data_detail_id'),
    ORMInt          ('ISP_AMOUNT',                    'isp_amount'),
    ORMInt          ('ISP_AMOUNT_GLEDGER',            'isp_amount_gledger'),
    ORMInt          ('ISP_AMOUNT_ORIGINAL',           'isp_amount_original'),
    ORMInt          ('PUB_AMOUNT',                    'pub_amount'),
    ORMInt          ('PUB_AMOUNT_GLEDGER',            'pub_amount_gledger'),
    ORMInt          ('PUB_AMOUNT_ORIGINAL',           'pub_amount_original'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_COMM_AMOUNT_ORIGINAL',      'pub_comm_amount_original'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )

invoiceinsertionorder = Object('INVOICEINSERTIONORDER', 'Invoiceinsertionorder', False)
invoiceinsertionorder.id = Index([
    ORMInt          ('CAMPAIGN_ALLOCATION_ID',        'campaign_allocation_id'),
    ORMInt          ('INVOICE_ID',                    'invoice_id'),
  ] )
invoiceinsertionorder.fields = [
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
  ]

ispcampaignstatsdailycredit = Object('ISPCAMPAIGNSTATSDAILYCREDIT', 'Ispcampaignstatsdailycredit', False)
ispcampaignstatsdailycredit.id = Index([
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMString       ('CCG_RATE_TYPE',                 'ccg_rate_type'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMInt          ('SIZE_ID',                       'size_id'),
  ] )
ispcampaignstatsdailycredit.fields = [
    ORMInt          ('CREDITED_ADV_AMOUNT_ISP',       'credited_adv_amount_isp'),
    ORMInt          ('CREDITED_CLICKS',               'credited_clicks'),
    ORMInt          ('CREDITED_IMPS',                 'credited_imps'),
  ]

ispstatsdailycredit = Object('ISPSTATSDAILYCREDIT', 'Ispstatsdailycredit', False)
ispstatsdailycredit.id = Index([
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
  ] )
ispstatsdailycredit.fields = [
    ORMInt          ('CREDITED_CLICKS',               'credited_clicks'),
    ORMInt          ('CREDITED_IMPS',                 'credited_imps'),
    ORMInt          ('CREDITED_ISP_AMOUNT',           'credited_isp_amount'),
    ORMInt          ('CREDITED_ISP_AMOUNT_GLOBAL',    'credited_isp_amount_global'),
  ]

objecttype = Object('OBJECTTYPE', 'ObjectType', False)
objecttype.id = Index([ORMInt('OBJECT_TYPE_ID', 'id')])
objecttype.fields = [
    ORMString       ('NAME',                          'name'),
  ]
objecttype.asks = [
    Select('name', ['NAME']),
  ]

foros_applied_patches_old = Object('OIX_APPLIED_PATCHES_OLD', 'Oix_applied_patches_old', False)
foros_applied_patches_old.id = Index([ORMString('PATCH_NAME', 'patch_name')])
foros_applied_patches_old.fields = [
    ORMDate         ('APPLY_DATE',                    'apply_date'),
    ORMTimestamp    ('START_DATE',                    'start_date'),
    ORMString       ('STATUS',                        'status'),
  ]

foros_timed_services = Object('OIX_TIMED_SERVICES', 'Oix_timed_services', False)
foros_timed_services.id = Index([ORMString('SERVICE_ID', 'service_id')])
foros_timed_services.fields = [
    ORMString       ('INSTANCE_ID',                   'instance_id'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

old_channeltrigger = Object('OLD_CHANNELTRIGGER', 'Old_channeltrigger', False)
old_channeltrigger.id = Index([ORMInt('CHANNEL_TRIGGER_ID', 'channel_trigger_id')])
old_channeltrigger.fields = [
    ORMInt          ('CHANNEL_ID',                    'channel_id', 'CHANNEL'),
    ORMString       ('MASKED',                        'masked'),
    ORMString       ('NEGATIVE',                      'negative'),
    ORMString       ('NORMALIZED_TRIGGER',            'normalized_trigger'),
    ORMString       ('ORIGINAL_TRIGGER',              'original_trigger'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMString       ('TRIGGER_GROUP',                 'trigger_group'),
    ORMInt          ('TRIGGER_ID',                    'trigger_id', 'OLD_TRIGGERS'),
    ORMString       ('TRIGGER_TYPE',                  'trigger_type'),
  ]

old_triggers = Object('OLD_TRIGGERS', 'Old_triggers', False)
old_triggers.id = Index([ORMInt('TRIGGER_ID', 'trigger_id')])
old_triggers.fields = [
    ORMString       ('CHANNEL_TYPE',                  'channel_type'),
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY'),
    ORMTimestamp    ('CREATED',                       'created'),
    ORMString       ('NORMALIZED_TRIGGER',            'normalized_trigger'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMString       ('TRIGGER_TYPE',                  'trigger_type'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMString       ('WILDCARD',                      'wildcard'),
  ]

optionenumvalue = Object('OPTIONENUMVALUE', 'Optionenumvalue', False)
optionenumvalue.id = Index([ORMInt('OPTION_ENUM_VALUE_ID', 'option_enum_value_id')], 'OPTIONENUMVALUESEQ')
optionenumvalue.fields = [
    ORMString       ('IS_DEFAULT',                    'is_default'),
    ORMString       ('NAME',                          'name'),
    ORMInt          ('OPTION_ID',                     'option_id', 'OPTIONS'),
    ORMString       ('VALUE',                         'value'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

optionfiletype = Object('OPTIONFILETYPE', 'Optionfiletype', False)
optionfiletype.id = Index([ORMInt('OPTIONFILETYPE_ID', 'optionfiletype_id')], 'OPTIONFILETYPESEQ')
optionfiletype.fields = [
    ORMString       ('FILE_TYPE',                     'file_type'),
    ORMInt          ('OPTION_ID',                     'option_id', 'OPTIONS'),
  ]

optiongroup = Object('OPTIONGROUP', 'Optiongroup', False)
optiongroup.id = Index([ORMInt('OPTION_GROUP_ID', 'option_group_id')], 'OPTIONGROUPSEQ')
optiongroup.fields = [
    ORMString       ('AVAILABILITY',                  'availability'),
    ORMString       ('COLLAPSIBILITY',                'collapsibility'),
    ORMString       ('LABEL',                         'label'),
    ORMString       ('NAME',                          'name'),
    ORMInt          ('SIZE_ID',                       'size_id', 'CREATIVESIZE'),
    ORMInt          ('SORT_ORDER',                    'sort_order'),
    ORMInt          ('TEMPLATE_ID',                   'template_id', 'TEMPLATE'),
    ORMString       ('TYPE',                          'type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

options = Object('OPTIONS', 'Options', False)
options.id = Index([ORMInt('OPTION_ID', 'option_id')], 'OPTIONSSEQ')
options.fields = [
    ORMString       ('DEFAULT_VALUE',                 'default_value'),
    ORMString       ('LABEL',                         'label'),
    ORMInt          ('MAX_VALUE',                     'max_value'),
    ORMInt          ('MIN_VALUE',                     'min_value'),
    ORMString       ('NAME',                          'name'),
    ORMInt          ('OPTION_GROUP_ID',               'option_group_id', 'OPTIONGROUP'),
    ORMInt          ('RECURSIVE_TOKENS',              'recursive_tokens'),
    ORMString       ('REQUIRED',                      'required'),
    ORMInt          ('SIZE_ID',                       'size_id'),
    ORMInt          ('SORT_ORDER',                    'sort_order'),
    ORMInt          ('TEMPLATE_ID',                   'template_id'),
    ORMString       ('TOKEN',                         'token'),
    ORMString       ('TYPE',                          'type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

pageloadsdaily = Object('PAGELOADSDAILY', 'Pageloadsdaily', False)
pageloadsdaily.id = Index([
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITE_ID',                       'site_id'),
    ORMString       ('TAG_GROUP',                     'tag_group'),
  ] )
pageloadsdaily.fields = [
    ORMInt          ('PAGE_LOADS',                    'page_loads'),
    ORMInt          ('UTILIZED_PAGE_LOADS',           'utilized_page_loads'),
  ]

pgchannelsuustatus = Object('PGCHANNELSUUSTATUS', 'Pgchannelsuustatus', False)
pgchannelsuustatus.id = Index([ORMInt('CHANNEL_ID', 'channel_id')])
pgchannelsuustatus.fields = [
    ORMString       ('STATUS',                        'status'),
  ]

platform = Object('PLATFORM', 'Platform', False)
platform.id = Index([ORMInt('PLATFORM_ID', 'platform_id')])
platform.fields = [
    ORMString       ('NAME',                          'name'),
    ORMString       ('TYPE',                          'type'),
  ]

platformdetector = Object('PLATFORMDETECTOR', 'Platformdetector', False)
platformdetector.id = Index([ORMInt('PLATFORM_DETECTOR_ID', 'platform_detector_id')], 'PLATFORMDETECTORSEQ')
platformdetector.fields = [
    ORMString       ('MATCH_MARKER',                  'match_marker'),
    ORMString       ('MATCH_REGEXP',                  'match_regexp'),
    ORMString       ('OUTPUT_REGEXP',                 'output_regexp'),
    ORMInt          ('PLATFORM_ID',                   'platform_id', 'PLATFORM'),
    ORMInt          ('PRIORITY',                      'priority'),
  ]

publisherstatsdaily = DualObject('PUBLISHERSTATSDAILY', 'Publisherstatsdaily', False)
publisherstatsdaily.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('INVALID_CLICKS',                'invalid_clicks'),
    ORMInt          ('INVALID_IMPS',                  'invalid_imps'),
    ORMInt          ('INVALID_REQUESTS',              'invalid_requests'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('PASSBACKS',                     'passbacks'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMInt          ('PUB_AMOUNT',                    'pub_amount'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_CREDITED_IMPS',             'pub_credited_imps'),
    ORMDate         ('PUB_SDATE',                     'pub_sdate'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMInt          ('SITE_ID',                       'site_id'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id'),
    ORMInt          ('SIZE_ID',                       'size_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMInt          ('TAG_PRICING_ID',                'tag_pricing_id'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

publisherstatstotal = Object('PUBLISHERSTATSTOTAL', 'Publisherstatstotal', False)
publisherstatstotal.id = Index([
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMInt          ('SITE_ID',                       'site_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
publisherstatstotal.fields = [
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('PASSBACKS',                     'passbacks'),
    ORMInt          ('PUB_AMOUNT',                    'pub_amount'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_CREDITED_IMPS',             'pub_credited_imps'),
    ORMInt          ('REQUESTS',                      'requests'),
  ]

replication_data = Object('REPLICATION_DATA', 'Replication_data', False)
replication_data.id = Index([ORMInt('MSG_ID', 'msg_id')])
replication_data.fields = [
    ORMString       ('DATA',                          'data'),
    ORMTimestamp    ('INSERT_TIMESTAMP',              'insert_timestamp'),
  ]

replication_heartbeat = DualObject('REPLICATION_HEARTBEAT', 'Replication_heartbeat', False)
replication_heartbeat.id = Index([ORMTimestamp('SOURCE_TIMESTAMP', 'source_timestamp')])

replication_marker = Object('REPLICATION_MARKER', 'Replication_marker', False)
replication_marker.id = Index([ORMInt('MARKER_ID', 'marker_id')], 'REPLICATION_MARKER_SEQ')
replication_marker.fields = [
    ORMString       ('MARKER_COMMENT',                'marker_comment'),
    ORMTimestamp    ('MARKER_TIMESTAMP',              'marker_timestamp'),
    ORMString       ('STATUS',                        'status'),
    ORMString       ('TYPE',                          'type'),
  ]

replication_table = Object('REPLICATION_TABLE', 'Replication_table', False)
replication_table.id = Index([
    ORMInt          ('STREAM_ID',                     'stream_id'),
    ORMString       ('TABLE_NAME',                    'table_name'),
  ] , 'REPLICATION_TABLE_SEQ')

report = Object('REPORT', 'Report', False)
report.id = Index([ORMInt('REPORT_ID', 'report_id')])
report.fields = [
    ORMString       ('BASE_TABLE',                    'base_table'),
    ORMString       ('BASE_TABLE_ALIAS',              'base_table_alias'),
    ORMString       ('DEBUG_MODE',                    'debug_mode'),
    ORMString       ('REPORT_NAME',                   'report_name'),
  ]

reportadditionalview = Object('REPORTADDITIONALVIEW', 'Reportadditionalview', False)
reportadditionalview.id = Index([ORMString('ADDITIONAL_VIEW_NAME', 'additional_view_name')])
reportadditionalview.fields = [
    ORMString       ('VIEW_STATEMENT',                'view_statement'),
    ORMString       ('WHERE_STATEMENT',               'where_statement'),
  ]

reportcolumn = Object('REPORTCOLUMN', 'Reportcolumn', False)
reportcolumn.id = Index([
    ORMString       ('COLUMN_ALIAS_ID',               'column_alias_id'),
    ORMString       ('RELATED_COLUMN',                'related_column'),
    ORMInt          ('REPORT_ID',                     'report_id'),
    ORMString       ('TABLE_ALIAS',                   'table_alias'),
  ] )
reportcolumn.fields = [
    ORMString       ('RELATED_COLUMN_STATEMENT',      'related_column_statement'),
  ]

reportcolumnalias = Object('REPORTCOLUMNALIAS', 'Reportcolumnalias', False)
reportcolumnalias.id = Index([
    ORMString       ('COLUMN_ALIAS_ID',               'column_alias_id'),
    ORMInt          ('REPORT_ID',                     'report_id'),
  ] )
reportcolumnalias.fields = [
    ORMString       ('ADDITIONAL_VIEW_NAME',          'additional_view_name'),
    ORMString       ('COLUMN_ALIAS',                  'column_alias'),
    ORMString       ('COLUMN_STATEMENT',              'column_statement'),
    ORMString       ('COLUMN_TYPE',                   'column_type'),
    ORMInt          ('INNER_LEVEL',                   'inner_level'),
    ORMString       ('VIEW_COLUMN_STATEMENT',         'view_column_statement'),
    ORMString       ('VIEW_NAME',                     'view_name'),
  ]

reporttable = Object('REPORTTABLE', 'Reporttable', False)
reporttable.id = Index([
    ORMInt          ('REPORT_ID',                     'report_id'),
    ORMString       ('TABLE_ALIAS',                   'table_alias'),
  ] )
reporttable.fields = [
    ORMString       ('PARENT_TABLE_ALIAS',            'parent_table_alias'),
    ORMString       ('TABLE_JOIN_STATEMENT',          'table_join_statement'),
    ORMString       ('TABLE_NAME',                    'table_name'),
  ]

reportviewjoincols = Object('REPORTVIEWJOINCOLS', 'Reportviewjoincols', False)
reportviewjoincols.id = Index([
    ORMString       ('COLUMN_ALIAS',                  'column_alias'),
    ORMString       ('VIEW_NAME',                     'view_name'),
  ] )
reportviewjoincols.fields = [
    ORMString       ('ADDITIONAL_VIEW_NAME',          'additional_view_name'),
    ORMString       ('COLUMN_ALIAS_VIEW',             'column_alias_view'),
    ORMString       ('TABLE_ALIAS',                   'table_alias'),
    ORMString       ('TABLE_NAME',                    'table_name'),
  ]

requeststatsdailycountry = DualObject('REQUESTSTATSDAILYCOUNTRY', 'Requeststatsdailycountry', False)
requeststatsdailycountry.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMFloat        ('ADV_AMOUNT',                    'adv_amount'),
    ORMFloat        ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMInt          ('ADV_INVOICE_COMM_AMOUNT',       'adv_invoice_comm_amount'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMFloat        ('ISP_AMOUNT',                    'isp_amount'),
    ORMFloat        ('ISP_AMOUNT_GLOBAL',             'isp_amount_global'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('PASSBACKS',                     'passbacks'),
    ORMInt          ('POSITION',                      'position'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMFloat        ('PUB_AMOUNT',                    'pub_amount'),
    ORMFloat        ('PUB_AMOUNT_GLOBAL',             'pub_amount_global'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_COMM_AMOUNT_GLOBAL',        'pub_comm_amount_global'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMString       ('USER_STATUS',                   'user_status'),
    ORMInt          ('VIRTUAL_AMOUNT_GLOBAL',         'virtual_amount_global'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

requeststatshourly = DualObject('REQUESTSTATSHOURLY', 'Requeststatshourly', False)
requeststatshourly.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMFloat        ('ADV_AMOUNT',                    'adv_amount'),
    ORMFloat        ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMInt          ('ADV_INVOICE_COMM_AMOUNT',       'adv_invoice_comm_amount'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMDate         ('GLOBAL_SDATE',                  'global_sdate'),
    ORMString       ('HID_PROFILE',                   'hid_profile'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMFloat        ('ISP_AMOUNT',                    'isp_amount'),
    ORMFloat        ('ISP_AMOUNT_GLOBAL',             'isp_amount_global'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('PASSBACKS',                     'passbacks'),
    ORMInt          ('POSITION',                      'position'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMFloat        ('PUB_AMOUNT',                    'pub_amount'),
    ORMFloat        ('PUB_AMOUNT_GLOBAL',             'pub_amount_global'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_COMM_AMOUNT_GLOBAL',        'pub_comm_amount_global'),
    ORMDate         ('PUB_SDATE',                     'pub_sdate'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMString       ('USER_STATUS',                   'user_status'),
    ORMInt          ('VIRTUAL_AMOUNT_GLOBAL',         'virtual_amount_global'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

requeststatshourlybatch = Object('REQUESTSTATSHOURLYBATCH', 'Requeststatshourlybatch', False)
requeststatshourlybatch.id = Index([ORMInt('BATCH_ID', 'batch_id')])
requeststatshourlybatch.fields = [
    ORMDate         ('BILLED_SDATE',                  'billed_sdate'),
    ORMString       ('BILLED_STATUS',                 'billed_status'),
    ORMInt          ('BILLING_DURATION',              'billing_duration'),
    ORMDate         ('LOADED_SDATE',                  'loaded_sdate'),
    ORMString       ('LOADED_STATUS',                 'loaded_status'),
    ORMInt          ('LOAD_DURATION',                 'load_duration'),
    ORMString       ('LOAD_TYPE',                     'load_type'),
    ORMInt          ('ROWS_NO',                       'rows_no'),
    ORMDate         ('UPDATED_DSTABLE_SDATE',         'updated_dstable_sdate'),
    ORMString       ('UPDATED_DSTABLE_STATUS',        'updated_dstable_status'),
    ORMDate         ('UPDATED_GSTABLE_SDATE',         'updated_gstable_sdate'),
    ORMString       ('UPDATED_GSTABLE_STATUS',        'updated_gstable_status'),
    ORMDate         ('UPDATED_HSTABLE_SDATE',         'updated_hstable_sdate'),
    ORMString       ('UPDATED_HSTABLE_STATUS',        'updated_hstable_status'),
    ORMInt          ('UPDATE_D_DURATION',             'update_d_duration'),
    ORMInt          ('UPDATE_G_DURATION',             'update_g_duration'),
    ORMInt          ('UPDATE_H_DURATION',             'update_h_duration'),
  ]

requeststatshourlystage = Object('REQUESTSTATSHOURLYSTAGE', 'Requeststatshourlystage', False)
requeststatshourlystage.id = Index([ORMInt('SH_ID', 'sh_id')])
requeststatshourlystage.fields = [
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMString       ('ADV_ACCOUNT_INVOICING_TYPE',    'adv_account_invoicing_type'),
    ORMString       ('ADV_ACCOUNT_PREPAY_TYPE',       'adv_account_prepay_type'),
    ORMString       ('ADV_ACCOUNT_RATE_TYPE',         'adv_account_rate_type'),
    ORMInt          ('ADV_AMOUNT',                    'adv_amount'),
    ORMInt          ('ADV_AMOUNT_CREDITED',           'adv_amount_credited'),
    ORMInt          ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_AMOUNT_INVOICED',           'adv_amount_invoiced'),
    ORMInt          ('ADV_AMOUNT_ISP',                'adv_amount_isp'),
    ORMInt          ('ADV_AMOUNT_ISP_CREDITED',       'adv_amount_isp_credited'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_CREDITED',      'adv_comm_amount_credited'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMInt          ('ADV_COMM_AMOUNT_INVOICED',      'adv_comm_amount_invoiced'),
    ORMInt          ('ADV_FIN_ACCOUNT_ID',            'adv_fin_account_id'),
    ORMInt          ('ADV_INVOICE_COMM_AMOUNT',       'adv_invoice_comm_amount'),
    ORMInt          ('ADV_INV_COMM_AMOUNT_CREDITED',  'adv_inv_comm_amount_credited'),
    ORMInt          ('ADV_INV_COMM_AMOUNT_INVOICED',  'adv_inv_comm_amount_invoiced'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMDate         ('ADV_SDATE_WITH_HOUR',           'adv_sdate_with_hour'),
    ORMInt          ('BATCH_ID',                      'batch_id'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCGRATE',                       'ccgrate'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate_id', 'CCGRATE'),
    ORMInt          ('CC_ID',                         'cc_id', 'CAMPAIGNCREATIVE'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMInt          ('CHANNEL_RATE_ID',               'channel_rate_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('COLO_ID',                       'colo_id', 'COLOCATION'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate_id', 'COLOCATIONRATE'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CTR_RESET_ID',                  'ctr_reset_id'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id', 'CURRENCYEXCHANGE'),
    ORMInt          ('DELIVERY_THRESHOLD',            'delivery_threshold'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
    ORMDate         ('END_BILLING_PERIOD',            'end_billing_period'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMDate         ('GLOBAL_SDATE',                  'global_sdate'),
    ORMString       ('HID_PROFILE',                   'hid_profile'),
    ORMInt          ('IMPS',                          'imps'),
    ORMString       ('ISCMP',                         'iscmp'),
    ORMString       ('ISCREDITED',                    'iscredited'),
    ORMString       ('ISCREDITEDWG',                  'iscreditedwg'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMInt          ('ISP_AMOUNT',                    'isp_amount'),
    ORMInt          ('ISP_AMOUNT_ADV',                'isp_amount_adv'),
    ORMInt          ('ISP_AMOUNT_CREDITED',           'isp_amount_credited'),
    ORMInt          ('ISP_AMOUNT_GLOBAL',             'isp_amount_global'),
    ORMInt          ('ISP_AMOUNT_GLOBAL_CREDITED',    'isp_amount_global_credited'),
    ORMInt          ('ISP_AMOUNT_INVOICED',           'isp_amount_invoiced'),
    ORMInt          ('ISP_IMP_CREDITED',              'isp_imp_credited'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('MIN_INVOICE',                   'min_invoice'),
    ORMString       ('NEEDS2NDLINE',                  'needs2ndline'),
    ORMString       ('NEEDS2NDLINEWG',                'needs2ndlinewg'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('PASSBACKS',                     'passbacks'),
    ORMInt          ('PAYABLE_ACCOUNT_ID',            'payable_account_id'),
    ORMInt          ('PAYABLE_AMOUNT',                'payable_amount'),
    ORMInt          ('PAYABLE_AMOUNT_ADV',            'payable_amount_adv'),
    ORMInt          ('PAYABLE_AMOUNT_ADV_CREDITED',   'payable_amount_adv_credited'),
    ORMInt          ('PAYABLE_AMOUNT_ADV_INVOICED',   'payable_amount_adv_invoiced'),
    ORMInt          ('PAYABLE_AMOUNT_CREDITED',       'payable_amount_credited'),
    ORMInt          ('PAYABLE_AMOUNT_GLOBAL',         'payable_amount_global'),
    ORMInt          ('PAYABLE_AMOUNT_INVOICED',       'payable_amount_invoiced'),
    ORMInt          ('PAYABLE_COMM_AMOUNT',           'payable_comm_amount'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_ADV',       'payable_comm_amount_adv'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_CREDITED',  'payable_comm_amount_credited'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_GLOBAL',    'payable_comm_amount_global'),
    ORMInt          ('PAYABLE_COMM_AMOUNT_INVOICED',  'payable_comm_amount_invoiced'),
    ORMDate         ('PAYABLE_SDATE',                 'payable_sdate'),
    ORMInt          ('POSITION',                      'position'),
    ORMString       ('PWP',                           'pwp'),
    ORMInt          ('QUANTITY',                      'quantity'),
    ORMInt          ('QUANTITY_CREDITED',             'quantity_credited'),
    ORMInt          ('QUANTITY_IMP_CREDITED',         'quantity_imp_credited'),
    ORMInt          ('QUANTITY_IMP_INVOICED',         'quantity_imp_invoiced'),
    ORMInt          ('QUANTITY_INVOICED',             'quantity_invoiced'),
    ORMString       ('RATE_TYPE',                     'rate_type'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITERATE',                      'siterate'),
    ORMString       ('SITERATETYPE',                  'siteratetype'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id', 'SITERATE'),
    ORMInt          ('SIZE_ID',                       'size_id', 'CREATIVESIZE'),
    ORMDate         ('START_BILLING_PERIOD',          'start_billing_period'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TAG_ID',                        'tag_id', 'TAGS'),
    ORMString       ('TEST',                          'test'),
    ORMString       ('UNITOFMEASURE',                 'unitofmeasure'),
    ORMString       ('USER_STATUS',                   'user_status'),
    ORMInt          ('VIRTUAL_AMOUNT_GLOBAL',         'virtual_amount_global'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ]

requeststatshourlytest = DualObject('REQUESTSTATSHOURLYTEST', 'Requeststatshourlytest', False)
requeststatshourlytest.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMFloat        ('ADV_AMOUNT',                    'adv_amount'),
    ORMFloat        ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMInt          ('ADV_INVOICE_COMM_AMOUNT',       'adv_invoice_comm_amount'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMInt          ('DEVICE_CHANNEL_ID',             'device_channel_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMDate         ('GLOBAL_SDATE',                  'global_sdate'),
    ORMString       ('HID_PROFILE',                   'hid_profile'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMFloat        ('ISP_AMOUNT',                    'isp_amount'),
    ORMFloat        ('ISP_AMOUNT_GLOBAL',             'isp_amount_global'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('PASSBACKS',                     'passbacks'),
    ORMInt          ('POSITION',                      'position'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMFloat        ('PUB_AMOUNT',                    'pub_amount'),
    ORMFloat        ('PUB_AMOUNT_GLOBAL',             'pub_amount_global'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_COMM_AMOUNT_GLOBAL',        'pub_comm_amount_global'),
    ORMDate         ('PUB_SDATE',                     'pub_sdate'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMString       ('USER_STATUS',                   'user_status'),
    ORMInt          ('VIRTUAL_AMOUNT_GLOBAL',         'virtual_amount_global'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

rtbcategory = Object('RTBCATEGORY', 'Rtbcategory', False)
rtbcategory.id = Index([ORMInt('RTB_CATEGORY_ID', 'rtb_category_id')], 'RTBCATEGORYSEQ')
rtbcategory.fields = [
    ORMInt          ('CREATIVE_CATEGORY_ID',          'creative_category_id', 'CREATIVECATEGORY'),
    ORMString       ('RTB_CATEGORY_KEY',              'rtb_category_key'),
    ORMInt          ('RTB_ID',                        'rtb_id', 'RTBCONNECTOR'),
  ]

rtbconnector = Object('RTBCONNECTOR', 'Rtbconnector', False)
rtbconnector.id = Index([ORMInt('RTB_ID', 'rtb_id')], 'RTBCONNECTORSEQ')
rtbconnector.fields = [
    ORMString       ('NAME',                          'name'),
  ]

schema_enabled_jobs = Object('SCHEMA_ENABLED_JOBS', 'Schema_enabled_jobs', False)
schema_enabled_jobs.id = Index([ORMString('JOB_NAME', 'job_name')])
schema_enabled_jobs.fields = [
    ORMString       ('JOB_STATUS',                    'job_status'),
    ORMDate         ('LAST_UPDATE_DATE',              'last_update_date'),
  ]

schema_reincarnations = DualObject('SCHEMA_REINCARNATIONS', 'Schema_reincarnations', False)
schema_reincarnations.id = Index([
    ORMTimestamp    ('CREATION_DATE',                 'creation_date'),
    ORMString       ('NAME',                          'name'),
  ] )

searchengine = Object('SEARCHENGINE', 'SearchEngine', False)
searchengine.id = Index([ORMInt('SEARCH_ENGINE_ID', 'search_engine_id')], 'SEARCHENGINESEQ')
searchengine.fields = [
    ORMInt          ('DECODING_DEPTH',                'decoding_depth'),
    ORMString       ('ENCODING',                      'encoding'),
    ORMString       ('HOST',                          'host'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('POST_ENCODING',                 'post_encoding'),
    ORMString       ('REGEXP',                        'regexp'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
searchengine.pgsync = True

selfbill = Object('SELFBILL', 'SelfBill', False)
selfbill.id = Index([ORMInt('BILL_ID', 'id')])
selfbill.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMDate         ('BILL_DATE',                     'bill_date'),
    ORMString       ('BILL_LEGAL_NUMBER',             'bill_legal_number'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('PAID_AMOUNT',                   'paid_amount'),
    ORMDate         ('PAYMENT_DATE',                  'payment_date'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TOTAL_AMOUNT',                  'total_amount'),
    ORMInt          ('TOTAL_AMOUNT_NET',              'total_amount_net'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

selfbill_addb_2040 = DualObject('SELFBILL_ADDB_2040', 'Selfbill_addb_2040', False)
selfbill_addb_2040.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMDate         ('BILL_DATE',                     'bill_date'),
    ORMInt          ('BILL_ID',                       'bill_id'),
    ORMString       ('BILL_LEGAL_NUMBER',             'bill_legal_number'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('PAID_AMOUNT',                   'paid_amount'),
    ORMDate         ('PAYMENT_DATE',                  'payment_date'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TAX_AMOUNT',                    'tax_amount'),
    ORMInt          ('TOTAL_AMOUNT',                  'total_amount'),
    ORMInt          ('TOTAL_AMOUNT_NET',              'total_amount_net'),
  ] )
selfbill_addb_2040.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

selfbilldaily = DualObject('SELFBILLDAILY', 'Selfbilldaily', False)
selfbilldaily.id = Index([
    ORMInt          ('AMOUNT_NET',                    'amount_net'),
    ORMInt          ('AMOUNT_NET_ORIGINAL',           'amount_net_original'),
    ORMInt          ('BILL_ID',                       'bill_id'),
    ORMInt          ('COMM_AMOUNT',                   'comm_amount'),
    ORMInt          ('COMM_AMOUNT_ORIGINAL',          'comm_amount_original'),
    ORMDate         ('IMP_DATE',                      'imp_date'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )

selfbilldata = Object('SELFBILLDATA', 'SelfBillData', False)
selfbilldata.id = Index([ORMInt('BILL_DATA_ID', 'id')])
selfbilldata.fields = [
    ORMInt          ('AMOUNT_NET',                    'amount_net'),
    ORMInt          ('BILL_ID',                       'bill', 'SELFBILL'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account', 'ACCOUNT'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

site = Object('SITE', 'Site', False)
site.id = Index([ORMInt('SITE_ID', 'id')], 'SITESEQ')
site.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMInt          ('DISPLAY_STATUS_ID',             'display_status_id', default = "1"),
    ORMInt          ('FLAGS',                         'flags'),
    ORMInt          ('FREQ_CAP_ID',                   'freq_cap', 'FREQCAP'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('NOTES',                         'notes'),
    ORMInt          ('NO_ADS_TIMEOUT',                'no_ads_timeout'),
    ORMDate         ('QA_DATE',                       'qa_date'),
    ORMString       ('QA_DESCRIPTION',                'qa_description'),
    ORMString       ('QA_STATUS',                     'qa_status'),
    ORMInt          ('QA_USER_ID',                    'qa_user_id', 'USERS'),
    ORMInt          ('SITE_CATEGORY_ID',              'site_category_id', 'SITECATEGORY'),
    ORMString       ('SITE_URL',                      'site_url'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
site.asks = [
    Select('index', ['NAME', 'ACCOUNT_ID' ]),
    Select('name', ['NAME']),
  ]
site.pgsync = True

sitecategory = Object('SITECATEGORY', 'Sitecategory', False)
sitecategory.id = Index([ORMInt('SITE_CATEGORY_ID', 'site_category_id')], 'SITECATEGORYSEQ')
sitecategory.fields = [
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY'),
    ORMString       ('NAME',                          'name'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
sitecategory.pgsync = True

sitecreativeapproval = Object('SITECREATIVEAPPROVAL', 'SiteCreativeApproval', False)
sitecreativeapproval.id = Index([
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('SITE_ID',                       'site_id'),
  ] )
sitecreativeapproval.fields = [
    ORMString       ('APPROVAL',                      'approval'),
    ORMTimestamp    ('APPROVAL_DATE',                 'approval_date'),
    ORMString       ('FEEDBACK',                      'feedback'),
    ORMInt          ('REJECT_REASON_ID',              'reject_reason_id', 'CREATIVEREJECTREASON'),
  ]
sitecreativeapproval.pgsync = True

sitecreativecategoryexclusion = Object('SITECREATIVECATEGORYEXCLUSION', 'SiteCreativeCategoryExclusion', False)
sitecreativecategoryexclusion.id = Index([
    ORMInt          ('CREATIVE_CATEGORY_ID',          'creative_category_id'),
    ORMInt          ('SITE_ID',                       'site_id'),
  ] )
sitecreativecategoryexclusion.fields = [
    ORMString       ('APPROVAL',                      'approval'),
  ]
sitecreativecategoryexclusion.pgsync = True

siterate = Object('SITERATE', 'SiteRate', False)
siterate.id = Index([ORMInt('SITE_RATE_ID', 'id')], 'SITERATESEQ')
siterate.fields = [
    ORMDate         ('EFFECTIVE_DATE',                'effective_date', default = "SYSDATE"),
    ORMFloat        ('RATE',                          'rate'),
    ORMString       ('RATE_TYPE',                     'rate_type'),
    ORMInt          ('TAG_PRICING_ID',                'tag_pricing', 'TAGPRICING'),
  ]
siterate.pgsync = True

sizetype = Object('SIZETYPE', 'SizeType', False)
sizetype.id = Index([ORMInt('SIZE_TYPE_ID', 'size_type_id')], 'SIZETYPESEQ')
sizetype.fields = [
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('TAG_TEMPLATE_FILE',             'tag_template_file'),
    ORMString       ('TAG_TEMPL_BRPB_FILE',           'tag_templ_brpb_file'),
    ORMString       ('TAG_TEMPL_IEST_FILE',           'tag_templ_iest_file'),
    ORMString       ('TAG_TEMPL_IFRAME_FILE',         'tag_templ_iframe_file'),
    ORMString       ('TAG_TEMPL_PREVIEW_FILE',        'tag_templ_preview_file'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
sizetype.pgsync = True

statshourly = DualObject('STATSHOURLY', 'Statshourly', False)
statshourly.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMFloat        ('ADV_AMOUNT',                    'adv_amount'),
    ORMFloat        ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMDate         ('GLOBAL_SDATE',                  'global_sdate'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_VIRTUAL',                  'imps_virtual'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMFloat        ('ISP_AMOUNT',                    'isp_amount'),
    ORMFloat        ('ISP_AMOUNT_GLOBAL',             'isp_amount_global'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('POSITION',                      'position'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMFloat        ('PUB_AMOUNT',                    'pub_amount'),
    ORMFloat        ('PUB_AMOUNT_GLOBAL',             'pub_amount_global'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_COMM_AMOUNT_GLOBAL',        'pub_comm_amount_global'),
    ORMDate         ('PUB_SDATE',                     'pub_sdate'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMString       ('USER_STATUS',                   'user_status'),
    ORMInt          ('VIRTUAL_AMOUNT_GLOBAL',         'virtual_amount_global'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

statshourlytest = DualObject('STATSHOURLYTEST', 'Statshourlytest', False)
statshourlytest.id = Index([
    ORMInt          ('ACTIONS',                       'actions'),
    ORMInt          ('ADV_ACCOUNT_ID',                'adv_account_id'),
    ORMFloat        ('ADV_AMOUNT',                    'adv_amount'),
    ORMFloat        ('ADV_AMOUNT_GLOBAL',             'adv_amount_global'),
    ORMInt          ('ADV_COMM_AMOUNT',               'adv_comm_amount'),
    ORMInt          ('ADV_COMM_AMOUNT_GLOBAL',        'adv_comm_amount_global'),
    ORMDate         ('ADV_SDATE',                     'adv_sdate'),
    ORMInt          ('CAMPAIGN_ID',                   'campaign_id'),
    ORMInt          ('CCG_ID',                        'ccg_id'),
    ORMInt          ('CCG_RATE_ID',                   'ccg_rate_id'),
    ORMInt          ('CC_ID',                         'cc_id'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('COLO_ID',                       'colo_id'),
    ORMInt          ('COLO_RATE_ID',                  'colo_rate_id'),
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('CURRENCY_EXCHANGE_ID',          'currency_exchange_id'),
    ORMString       ('FRAUD_CORRECTION',              'fraud_correction'),
    ORMDate         ('GLOBAL_SDATE',                  'global_sdate'),
    ORMInt          ('IMPS',                          'imps'),
    ORMInt          ('IMPS_VIRTUAL',                  'imps_virtual'),
    ORMInt          ('ISP_ACCOUNT_ID',                'isp_account_id'),
    ORMFloat        ('ISP_AMOUNT',                    'isp_amount'),
    ORMFloat        ('ISP_AMOUNT_GLOBAL',             'isp_amount_global'),
    ORMDate         ('ISP_SDATE',                     'isp_sdate'),
    ORMInt          ('NUM_SHOWN',                     'num_shown'),
    ORMInt          ('POSITION',                      'position'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id'),
    ORMFloat        ('PUB_AMOUNT',                    'pub_amount'),
    ORMFloat        ('PUB_AMOUNT_GLOBAL',             'pub_amount_global'),
    ORMInt          ('PUB_COMM_AMOUNT',               'pub_comm_amount'),
    ORMInt          ('PUB_COMM_AMOUNT_GLOBAL',        'pub_comm_amount_global'),
    ORMDate         ('PUB_SDATE',                     'pub_sdate'),
    ORMInt          ('REQUESTS',                      'requests'),
    ORMDate         ('SDATE',                         'sdate'),
    ORMInt          ('SITE_RATE_ID',                  'site_rate_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
    ORMString       ('USER_STATUS',                   'user_status'),
    ORMInt          ('VIRTUAL_AMOUNT_GLOBAL',         'virtual_amount_global'),
    ORMString       ('WALLED_GARDEN',                 'walled_garden'),
  ] )

tag_tagsize = Object('TAG_TAGSIZE', 'Tag_tagsize', False)
tag_tagsize.id = Index([
    ORMInt          ('SIZE_ID',                       'size_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
tag_tagsize.pgsync = True

tagauctionsettings = Object('TAGAUCTIONSETTINGS', 'Tagauctionsettings', False)
tagauctionsettings.id = Index([ORMInt('TAG_ID', 'tag_id')])
tagauctionsettings.fields = [
    ORMFloat        ('MAX_ECPM_SHARE',                'max_ecpm_share'),
    ORMFloat        ('PROP_PROBABILITY_SHARE',        'prop_probability_share'),
    ORMFloat        ('RANDOM_SHARE',                  'random_share'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

tagcontentcategory = DualObject('TAGCONTENTCATEGORY', 'Tagcontentcategory', False)
tagcontentcategory.id = Index([
    ORMInt          ('CONTENT_CATEGORY_ID',           'content_category_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )

tagctr = Object('TAGCTR', 'Tagctr', False)
tagctr.id = Index([
    ORMString       ('COUNTRY_CODE',                  'country_code'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
tagctr.fields = [
    ORMInt          ('ADJUSTMENT',                    'adjustment'),
    ORMInt          ('CLICKS',                        'clicks'),
    ORMInt          ('CTR',                           'ctr'),
    ORMInt          ('IMPS',                          'imps'),
  ]

tagctroverride = Object('TAGCTROVERRIDE', 'Tagctroverride', False)
tagctroverride.id = Index([ORMInt('TAG_ID', 'tag_id')])
tagctroverride.fields = [
    ORMInt          ('ADJUSTMENT',                    'adjustment'),
  ]

tagoptgroupstate = Object('TAGOPTGROUPSTATE', 'Tagoptgroupstate', False)
tagoptgroupstate.id = Index([
    ORMInt          ('OPTION_GROUP_ID',               'option_group_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
tagoptgroupstate.fields = [
    ORMString       ('COLLAPSED',                     'collapsed'),
    ORMString       ('ENABLED',                       'enabled'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

tagoptionvalue = Object('TAGOPTIONVALUE', 'Tagoptionvalue', False)
tagoptionvalue.id = Index([
    ORMInt          ('OPTION_ID',                     'option_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
tagoptionvalue.fields = [
    ORMString       ('VALUE',                         'value'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

tagpricing = Object('TAGPRICING', 'TagPricing', False)
tagpricing.id = Index([ORMInt('TAG_PRICING_ID', 'id')], 'TAGPRICINGSEQ')
tagpricing.fields = [
    ORMString       ('CCG_RATE_TYPE',                 'ccg_rate_type'),
    ORMString       ('CCG_TYPE',                      'ccg_type'),
    ORMString       ('COUNTRY_CODE',                  'country_code', 'COUNTRY', default = "'GN'"),
    ORMInt          ('SITE_RATE_ID',                  'site_rate', 'SITERATE'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TAG_ID',                        'tag', 'TAGS'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
tagpricing.pgsync = True

tags = Object('TAGS', 'Tags', False)
tags.id = Index([ORMInt('TAG_ID', 'id')], 'TAGSSEQ')
tags.fields = [
    ORMString       ('ALLOW_EXPANDABLE',              'allow_expandable'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('MARKETPLACE',                   'marketplace'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('PASSBACK',                      'passback'),
    ORMString       ('PASSBACK_CODE',                 'passback_code'),
    ORMString       ('PASSBACK_TYPE',                 'passback_type'),
    ORMInt          ('SITE_ID',                       'site', 'SITE'),
    ORMInt          ('SIZE_TYPE_ID',                  'size_type_id', 'SIZETYPE'),
    ORMString       ('STATUS',                        'status'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
tags.asks = [
    Select('name', ['NAME']),
  ]
tags.pgsync = True

tagscreativecategoryexclusion = Object('TAGSCREATIVECATEGORYEXCLUSION', 'Tagscreativecategoryexclusion', False)
tagscreativecategoryexclusion.id = Index([
    ORMInt          ('CREATIVE_CATEGORY_ID',          'creative_category_id'),
    ORMInt          ('TAG_ID',                        'tag_id'),
  ] )
tagscreativecategoryexclusion.fields = [
    ORMString       ('APPROVAL',                      'approval'),
  ]
tagscreativecategoryexclusion.pgsync = True

template = Object('TEMPLATE', 'Template', False)
template.id = Index([ORMInt('TEMPLATE_ID', 'template_id')], 'TEMPLATESEQ')
template.fields = [
    ORMString       ('EXPANDABLE',                    'expandable'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('STATUS',                        'status'),
    ORMString       ('TEMPLATE_TYPE',                 'template_type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
template.pgsync = True

templatefile = Object('TEMPLATEFILE', 'Templatefile', False)
templatefile.id = Index([ORMInt('TEMPLATE_FILE_ID', 'template_file_id')], 'TEMPLATEFILESEQ')
templatefile.fields = [
    ORMInt          ('APP_FORMAT_ID',                 'app_format_id', 'APPFORMAT'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMInt          ('SIZE_ID',                       'size_id', 'CREATIVESIZE'),
    ORMString       ('TEMPLATE_FILE',                 'template_file'),
    ORMInt          ('TEMPLATE_ID',                   'template_id', 'TEMPLATE'),
    ORMString       ('TEMPLATE_TYPE',                 'template_type'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
templatefile.pgsync = True

test = DualObject('TEST', 'Test', False)
test.id = Index([
    ORMInt          ('A',                             'a'),
    ORMInt          ('B',                             'b'),
  ] )

thirdpartycreative = Object('THIRDPARTYCREATIVE', 'Thirdpartycreative', False)
thirdpartycreative.id = Index([
    ORMInt          ('CREATIVE_ID',                   'creative_id'),
    ORMInt          ('SITE_ID',                       'site_id'),
  ] )
thirdpartycreative.fields = [
    ORMString       ('THIRD_PARTY_APPROVAL',          'third_party_approval'),
    ORMString       ('THIRD_PARTY_CREATIVE_ID',       'third_party_creative_id'),
  ]

timedimension = Object('TIMEDIMENSION', 'Timedimension', False)
timedimension.id = Index([ORMDate('SDATE', 'sdate')])
timedimension.fields = [
    ORMInt          ('DAY_NUMBER',                    'day_number'),
    ORMString       ('DAY_OF_THE_WEEK',               'day_of_the_week'),
    ORMInt          ('MONTH',                         'month'),
    ORMString       ('MONTH_NAME',                    'month_name'),
    ORMInt          ('QUARTER',                       'quarter'),
    ORMInt          ('WEEK_NUMBER',                   'week_number'),
    ORMInt          ('YEAR',                          'year'),
  ]

timezone = Object('TIMEZONE', 'Timezone', False)
timezone.id = Index([ORMInt('TIMEZONE_ID', 'timezone_id')])
timezone.fields = [
    ORMString       ('DESCRIPTION',                   'description'),
    ORMString       ('STAT_SUFFIX',                   'stat_suffix'),
    ORMString       ('TZNAME',                        'tzname'),
  ]
timezone.pgsync = True

updchanneldatafrompg = DualObject('UPDCHANNELDATAFROMPG', 'Updchanneldatafrompg', False)
updchanneldatafrompg.id = Index([
    ORMClob         ('AUDIT_LOG_ENTRY',               'audit_log_entry'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMInt          ('DISTINCT_URL_TRIGGERS_COUNT',   'distinct_url_triggers_count'),
    ORMString       ('TRIGGERS_STATUS',               'triggers_status'),
  ] )

updchanneldeclinationfrompg = DualObject('UPDCHANNELDECLINATIONFROMPG', 'Updchanneldeclinationfrompg', False)
updchanneldeclinationfrompg.id = Index([
    ORMInt          ('CHANNEL_ACCOUNT_ID',            'channel_account_id'),
    ORMInt          ('CHANNEL_ID',                    'channel_id'),
    ORMString       ('CHANNEL_NAME',                  'channel_name'),
    ORMString       ('CHANNEL_TYPE',                  'channel_type'),
    ORMString       ('DATA_KIND',                     'data_kind'),
    ORMString       ('DATA_VALUE',                    'data_value'),
    ORMInt          ('MAX_URL_TRIGGER_SHARE',         'max_url_trigger_share'),
  ] )

updstatsstatusfrompg = DualObject('UPDSTATSSTATUSFROMPG', 'Updstatsstatusfrompg', False)
updstatsstatusfrompg.id = Index([
    ORMInt          ('OBJECT_ID',                     'object_id'),
    ORMString       ('STATS_STATUS',                  'stats_status'),
  ] )

useradvertiser = Object('USERADVERTISER', 'UserAdvertiser', False)
useradvertiser.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('USER_ID',                       'user_id'),
  ] )

usercredentials = Object('USERCREDENTIALS', 'Usercredentials', False)
usercredentials.id = Index([ORMInt('USER_CREDENTIAL_ID', 'user_credential_id')], 'USERCREDENTIALSSEQ')
usercredentials.fields = [
    ORMTimestamp    ('BLOCKED_UNTIL',                 'blocked_until'),
    ORMString       ('EMAIL',                         'email'),
    ORMDate         ('LAST_LOGIN_DATE',               'last_login_date'),
    ORMString       ('LAST_LOGIN_IP',                 'last_login_ip'),
    ORMString       ('PASSWORD',                      'password'),
    ORMString       ('RS_AUTH_TOKEN',                 'rs_auth_token'),
    ORMString       ('RS_SIGNATURE_KEY',              'rs_signature_key'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMInt          ('WRONG_ATTEMPTS',                'wrong_attempts'),
  ]
usercredentials.pgsync = True

userrole = Object('USERROLE', 'UserRole', False)
userrole.id = Index([ORMInt('USER_ROLE_ID', 'id')], 'USERROLESEQ')
userrole.fields = [
    ORMInt          ('ACCOUNT_ROLE_ID',               'account_role', 'ACCOUNTROLE'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('INTERNAL_ACCESS_TYPE',          'internal_access_type'),
    ORMString       ('LDAP_DN',                       'ldap_dn'),
    ORMString       ('NAME',                          'name'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
userrole.asks = [
    Select('name', ['NAME']),
  ]
userrole.pgsync = True

userroleinternalaccess = Object('USERROLEINTERNALACCESS', 'Userroleinternalaccess', False)
userroleinternalaccess.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('USER_ROLE_ID',                  'user_role_id'),
  ] )

users = Object('USERS', 'Users', False)
users.id = Index([ORMInt('USER_ID', 'id')], 'USERSEQ')
users.fields = [
    ORMInt          ('ACCOUNT_ID',                    'account', 'ACCOUNT'),
    ORMInt          ('ADDRESS_ID',                    'address', 'ACCOUNTADDRESS'),
    ORMString       ('AUTH_TYPE',                     'auth_type'),
    ORMString       ('EMAIL',                         'email'),
    ORMString       ('FIRST_NAME',                    'first_name'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('JOB_TITLE',                     'job_title'),
    ORMString       ('LANGUAGE',                      'language'),
    ORMString       ('LAST_NAME',                     'last_name'),
    ORMString       ('LDAP_DN',                       'ldap_dn'),
    ORMString       ('PHONE',                         'phone'),
    ORMFloat        ('PREPAID_AMOUNT_LIMIT',          'prepaid_amount_limit'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('USER_CREDENTIAL_ID',            'user_credential_id', 'USERCREDENTIALS'),
    ORMInt          ('USER_ROLE_ID',                  'user_role', 'USERROLE'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
users.pgsync = True

users_old = DualObject('USERS_OLD', 'Users_old', False)
users_old.id = Index([
    ORMInt          ('ACCOUNT_ID',                    'account_id'),
    ORMInt          ('ADDRESS_ID',                    'address_id'),
    ORMTimestamp    ('BLOCKED_UNTIL',                 'blocked_until'),
    ORMString       ('EMAIL',                         'email'),
    ORMString       ('FIRST_NAME',                    'first_name'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('JOB_TITLE',                     'job_title'),
    ORMString       ('LANGUAGE',                      'language'),
    ORMString       ('LAST_NAME',                     'last_name'),
    ORMString       ('LDAP_DN',                       'ldap_dn'),
    ORMString       ('LOGIN',                         'login'),
    ORMString       ('PASSWORD',                      'password'),
    ORMString       ('PHONE',                         'phone'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('USER_ID',                       'user_id'),
    ORMInt          ('USER_ROLE_ID',                  'user_role_id'),
    ORMInt          ('WRONG_ATTEMPTS',                'wrong_attempts'),
  ] )
users_old.fields = [
    ORMTimestamp    ('VERSION',                       'version'),
  ]

usersite = Object('USERSITE', 'Usersite', False)
usersite.id = Index([
    ORMInt          ('SITE_ID',                       'site_id'),
    ORMInt          ('USER_ID',                       'user_id'),
  ] )

walledgarden = Object('WALLEDGARDEN', 'Walledgarden', False)
walledgarden.id = Index([ORMInt('WG_ID', 'wg_id')], 'WALLEDGARDENSEQ')
walledgarden.fields = [
    ORMInt          ('AGENCY_ACCOUNT_ID',             'agency_account_id', 'ACCOUNT'),
    ORMString       ('AGENCY_MARKETPLACE',            'agency_marketplace'),
    ORMInt          ('PUB_ACCOUNT_ID',                'pub_account_id', 'ACCOUNT'),
    ORMString       ('PUB_MARKETPLACE',               'pub_marketplace'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

wdrequestmapping = Object('WDREQUESTMAPPING', 'Wdrequestmapping', False)
wdrequestmapping.id = Index([ORMInt('WD_REQ_MAPPING_ID', 'wd_req_mapping_id')], 'WDREQUESTMAPPINGSEQ')
wdrequestmapping.fields = [
    ORMString       ('DESCRIPTION',                   'description'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('REQUEST',                       'request'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]
wdrequestmapping.pgsync = True

wdtag = Object('WDTAG', 'WDTag', False)
wdtag.id = Index([ORMInt('WDTAG_ID', 'wdtag_id')], 'TAGSSEQ')
wdtag.fields = [
    ORMInt          ('HEIGHT',                        'height'),
    ORMString       ('NAME',                          'name'),
    ORMString       ('OPTED_IN_CONTENT',              'opted_in_content'),
    ORMString       ('OPTED_OUT_CONTENT',             'opted_out_content'),
    ORMString       ('PASSBACK',                      'passback'),
    ORMInt          ('SITE_ID',                       'site_id', 'SITE'),
    ORMString       ('STATUS',                        'status'),
    ORMInt          ('TEMPLATE_ID',                   'template_id', 'TEMPLATE'),
    ORMTimestamp    ('VERSION',                       'version'),
    ORMInt          ('WIDTH',                         'width'),
  ]

wdtagfeed_optedin = Object('WDTAGFEED_OPTEDIN', 'WDTagfeed_optedin', False)
wdtagfeed_optedin.id = Index([
    ORMInt          ('FEED_ID',                       'feed_id'),
    ORMInt          ('WDTAG_ID',                      'wdtag_id'),
  ] )

wdtagfeed_optedout = Object('WDTAGFEED_OPTEDOUT', 'WDTagfeed_optedout', False)
wdtagfeed_optedout.id = Index([
    ORMInt          ('FEED_ID',                       'feed_id'),
    ORMInt          ('WDTAG_ID',                      'wdtag_id'),
  ] )

wdtagoptgroupstate = Object('WDTAGOPTGROUPSTATE', 'Wdtagoptgroupstate', False)
wdtagoptgroupstate.id = Index([
    ORMInt          ('OPTION_GROUP_ID',               'option_group_id'),
    ORMInt          ('WDTAG_ID',                      'wdtag_id'),
  ] )
wdtagoptgroupstate.fields = [
    ORMString       ('COLLAPSED',                     'collapsed'),
    ORMString       ('ENABLED',                       'enabled'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

wdtagoptionvalue = Object('WDTAGOPTIONVALUE', 'Wdtagoptionvalue', False)
wdtagoptionvalue.id = Index([
    ORMInt          ('OPTION_ID',                     'option_id'),
    ORMInt          ('WDTAG_ID',                      'wdtag_id'),
  ] )
wdtagoptionvalue.fields = [
    ORMString       ('VALUE',                         'value'),
    ORMTimestamp    ('VERSION',                       'version'),
  ]

webapplication = Object('WEBAPPLICATION', 'Webapplication', False)
webapplication.id = Index([ORMString('NAME', 'name')])
webapplication.fields = [
    ORMString       ('STATUS',                        'status'),
  ]

webapplicationoperation = Object('WEBAPPLICATIONOPERATION', 'Webapplicationoperation', False)
webapplicationoperation.id = Index([ORMString('NAME', 'name')])
webapplicationoperation.fields = [
    ORMString       ('STATUS',                        'status'),
  ]

webapplicationsource = Object('WEBAPPLICATIONSOURCE', 'Webapplicationsource', False)
webapplicationsource.id = Index([ORMString('NAME', 'name')])
webapplicationsource.fields = [
    ORMString       ('STATUS',                        'status'),
  ]

webbrowser = DualObject('WEBBROWSER', 'Webbrowser', False)
webbrowser.id = Index([
    ORMString       ('MARKER',                        'marker'),
    ORMString       ('NAME',                          'name'),
    ORMInt          ('PRIORITY',                      'priority'),
    ORMString       ('REGEXP',                        'regexp'),
    ORMString       ('REGEXP_REQUIRED',               'regexp_required'),
  ] )

weboperation = Object('WEBOPERATION', 'Weboperation', False)
weboperation.id = Index([ORMInt('WEB_OPERATION_ID', 'web_operation_id')])
weboperation.fields = [
    ORMString       ('APP',                           'app'),
    ORMInt          ('FLAGS',                         'flags'),
    ORMString       ('OPERATION',                     'operation'),
    ORMString       ('SOURCE',                        'source'),
    ORMString       ('STATUS',                        'status'),
  ]

objects = [
    account,
    accountaddress,
    accountfinancialdata,
    accountfinancialdata_addb_2040,
    accountfinancialsettings,
    accountrole,
    accounttype,
    accounttype_ccgtype,
    accounttypecreativesize,
    accounttypecreativetemplate,
    accounttypedevicechannel,
    action,
    actiontype,
    adsconfig,
    advertiserstats,
    advertiserstatsdaily,
    advertiseruserstats,
    advertiseruserstatsrunning,
    advertiseruserstatstotal,
    advpubstatsdaily,
    advpubstatstotal,
    appformat,
    auctionsettings,
    authenticationtoken,
    bash_tmp,
    behavioralparameters,
    behavioralparameterslist,
    birtreport,
    birtreportinstance,
    birtreportsession,
    campaign,
    campaignallocation,
    campaignallocationusage,
    campaigncreative,
    campaigncreativegroup,
    campaigncredit,
    campaigncreditallocation,
    campaigncreditallocationusage,
    campaigncreditstatsdaily,
    campaignexcludedchannel,
    campaignschedule,
    campaignuserstats,
    campaignuserstatsrunning,
    campaignuserstatstotal,
    ccgaction,
    ccgaroverride,
    ccgcolocation,
    ccgctroverride,
    ccgdevicechannel,
    ccggeochannel,
    ccghistoryctr,
    ccgkeyword,
    ccgkeywordctroverride,
    ccgkeywordhistoryctr,
    ccgkeywordstatsdaily,
    ccgkeywordstatshourly,
    ccgkeywordstatstotal,
    ccgkeywordstatstow,
    ccgmobileopchannel,
    ccgrate,
    ccgschedule,
    ccgsite,
    ccgtaghourlystats,
    ccguserstats,
    ccguserstatsrunning,
    ccguserstatstotal,
    ccuserstats,
    ccuserstatsrunning,
    ccuserstatstotal,
    changedobject,
    channel,
    channelcategory,
    channelimpinventory,
    channelinventory,
    channelrate,
    check_,
    cmprequeststatshourly,
    colocation,
    colocationrate,
    colousers,
    colouserstats,
    contentcategory,
    conversioncategory,
    country,
    countryaddressfield,
    creative,
    creative_tagsize,
    creative_tagsizetype,
    creativecategory,
    creativecategory_creative,
    creativecategory_template,
    creativecategorytype,
    creativeoptgroupstate,
    creativeoptionvalue,
    creativerejectreason,
    creativesize,
    creativesizeexpansion,
    ctr_chg_log,
    ctr_kw_tow_matrix,
    ctr_kwtg_log,
    ctr_pta_log,
    ctralgadvertiserexclusion,
    ctralgcampaignexclusion,
    ctralgorithm,
    ctrchggtt,
    ctrhistory,
    ctrkwtggtt,
    ctrkwtgtowgtt,
    ctrptagtt,
    currency,
    currencyexchange,
    currencyexchangerate,
    displaystatus,
    expressionusedchannel,
    feed,
    fraudcondition,
    freqcap,
    globalcolousers,
    globalcolouserstats,
    historyctrstatstotal,
    insertionorder,
    invoice,
    invoice_addb1895,
    invoice_addb_2040,
    invoicedata,
    invoicedata_addb1895,
    invoicedatadetail,
    invoicedatadetail_addb1895,
    invoicedatadetail_addb_2040,
    invoicedatadetail_addb_2144,
    invoicehourly_addb_2144,
    invoiceinsertionorder,
    ispcampaignstatsdailycredit,
    ispstatsdailycredit,
    objecttype,
    foros_applied_patches_old,
    foros_timed_services,
    old_channeltrigger,
    old_triggers,
    optionenumvalue,
    optionfiletype,
    optiongroup,
    options,
    pageloadsdaily,
    pgchannelsuustatus,
    platform,
    platformdetector,
    publisherstatsdaily,
    publisherstatstotal,
    replication_data,
    replication_heartbeat,
    replication_marker,
    replication_table,
    report,
    reportadditionalview,
    reportcolumn,
    reportcolumnalias,
    reporttable,
    reportviewjoincols,
    requeststatsdailycountry,
    requeststatshourly,
    requeststatshourlybatch,
    requeststatshourlystage,
    requeststatshourlytest,
    rtbcategory,
    rtbconnector,
    schema_enabled_jobs,
    schema_reincarnations,
    searchengine,
    selfbill,
    selfbill_addb_2040,
    selfbilldaily,
    selfbilldata,
    site,
    sitecategory,
    sitecreativeapproval,
    sitecreativecategoryexclusion,
    siterate,
    sizetype,
    statshourly,
    statshourlytest,
    tag_tagsize,
    tagauctionsettings,
    tagcontentcategory,
    tagctr,
    tagctroverride,
    tagoptgroupstate,
    tagoptionvalue,
    tagpricing,
    tags,
    tagscreativecategoryexclusion,
    template,
    templatefile,
    test,
    thirdpartycreative,
    timedimension,
    timezone,
    updchanneldatafrompg,
    updchanneldeclinationfrompg,
    updstatsstatusfrompg,
    useradvertiser,
    usercredentials,
    userrole,
    userroleinternalaccess,
    users,
    users_old,
    usersite,
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
]

unimportant = [
    'AUDITLOG',
    'CAPABILITYTESTS',
    'CHANGE_PASSWORD_UID',
    'CLOBPARAMS',
    'CUSTOMREPORT',
    'DYNAMICRESOURCES',
    'ERRORLOG',
    'ERRORMESSAGE',
    'INTERFACEDATASENT',
    'MADECHANGE',
    'OIX_APPLIED_PATCHES',
    'ORACLE_JOB',
    'PAYMENT',
    'POLICY',
    'SCHEMA_APPLIED_PATCHES',
    'WIKI_ATT',
    'WIKI_PAGE',
    'XXPHM_GL01_OIX_MASTER',
    'XXPHM_AR25_ORA_ACCT_CREDIT',
    'ACTIONSTATS',
    'CAMPAIGNSTATS',
    'CCGKEYWORDSTATS',
    'CCGSTATS',
    'CCSTATS',
    'CHANNELINVENTORYESTIMSTATS',
    'CHANNELTRIGGERSTATS',
    'CHANNELUSAGESTATS',
    'COLOSTATS',
    'KEYWORDSTATS',
    'MARGINRULESTATSDAILY',
    'OPTOUTSTATS',
    'PASSBACKSTATS',
    'REQUESTTRIGGERCHANNELSTATS',
    'REQUESTTRIGGERSTATS',
    'SITECHANNELSTATS',
    'SITEREFERRERSTATS',
    'STATSHOURLYSTAGE',
    'WEBWISEDISCOVERITEMSTATS',
    'WEBWISEDISCOVERTAGSTATS',
    'SITEUSERSTATS',
    'CCGSTATSDAILY',
    'CCSTATSDAILY',
    'REQUESTSTATSDAILYBR',
    'REQUESTSTATSDAILYGB',
    'REQUESTSTATSDAILYGMT',
    'REQUESTSTATSDAILYISP',
    'REQUESTSTATSDAILYTR',
    'UUGENLOG',
]
