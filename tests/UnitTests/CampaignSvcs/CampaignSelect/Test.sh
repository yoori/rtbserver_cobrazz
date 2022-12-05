#!/bin/sh

function killed
{
  kill 0
  exit
}

trap killed SIGTERM

echo Oracle Campaign Select test started.

db="//oraads/addbads.ocslab.com"
user="ads_3"
pass="adserver"
valgrind_prefix="${3}"

if test "$#" -eq "0"; then
  iterations=1
else
  iterations=$1
fi

if test "$#" -le "1"; then
  delay=2 # seconds
else
  delay=$2
fi

query="SELECT
  ccg.campaign_id campaign_id,
  ccg.ccg_id ccg_id,
  ccg.ccg_rate_id ccg_rate_id,
  crate.rate_type,
  c.freq_cap_id campaign_fc_id,
  ccg.freq_cap_id ccg_fc_id,
  ccg.country_code,
  ccg.flags flags,
  nvl(c.marketplace, 'OIX'),
  nvl(crate.cpm, 0) / 1000,
  nvl(crate.cpc, 0),
  nvl(crate.cpa, 0),
  nvl(c.commission, 0),
  nvl(ag.account_id, ac.account_id),
  ac.account_id,
  ccg.ccg_type,
  ccg.tgt_type,
  ccg.optin_status_targeting,
  ccg.ctr_reset_id,
  (case when nvl(hcs.imps, 0) >= ctralg.cpc_random_imps then 1 else 0 end),
  ccg.min_uid_age,
  nvl(ccg.rotation_criteria, 0),
  ccg.channel_id,
  c.date_start,
  c.date_end,
  c.budget,
  (CASE WHEN c.daily_budget <> 0 THEN c.daily_budget ELSE c.daily_budget END),
  c.delivery_pacing,
  ccg.date_start,
  CASE getbit(ccg.flags, 8)
    WHEN 0 THEN (
      CASE WHEN (c.date_end > ccg.date_end) then ccg.date_end
      WHEN (c.date_end <= ccg.date_end) then c.date_end
      ELSE ccg.date_end END)
    ELSE c.date_end END,
  (CASE WHEN ccg.budget <> 0 THEN ccg.budget ELSE NULL END),
  (CASE WHEN ccg.daily_budget <> 0 THEN ccg.daily_budget ELSE NULL END),
  ccg.delivery_pacing,
  trunc((CASE WHEN crate.rate_type = 'CPC' THEN
    nvl(ctr.ctr, 0) * crate.cpc * 1000 / cer.rate
    ELSE old_ecpm.ecpm END) * 100, 8),
  trunc((CASE WHEN crate.rate_type = 'CPC' THEN
    nvl(ctr.ctr, 0) ELSE 0 END), 8),
  nvl(c.max_pub_share, 0),
  adserverutil.cross_status(
    adserverutil.cross_status(ag.adserver_status, ac.adserver_status),
    adserverutil.cross_status(
      adserverutil.cross_status(ccg.adserver_status, c.status),
      case when (getbit(ccg.flags,11) = 1) then 'V' else 'A' end
    )
  ),
  ac.currency_id,
  nvl2(ccg.user_sample_group_start, ccg.user_sample_group_start - 1, 0),
  nvl(ccg.user_sample_group_end, 100)
FROM
  v_CampaignCreativeGroup ccg
    INNER JOIN Campaign c ON c.campaign_id = ccg.campaign_id
    INNER JOIN v_Account ac ON ac.account_id = c.account_id
    INNER JOIN CCGRate crate ON (crate.ccg_rate_id = ccg.ccg_rate_id)
    LEFT JOIN v_Account ag ON(ag.account_id = ac.agency_account_id)
    LEFT JOIN CCGHistoryCTR ctr ON(ctr.ccg_id = ccg.ccg_id)
    LEFT JOIN v_ECPM old_ecpm ON(old_ecpm.ccg_id = ccg.ccg_id)
    LEFT JOIN CtrAlgorithm ctralg ON(ccg.country_code = ctralg.country_code)
    LEFT JOIN HistoryCTRStatsTotal hcs ON(hcs.ccg_id = ccg.ccg_id AND hcs.ctr_reset_id = ccg.ctr_reset_id),
  CurrencyExchangeRate cer
WHERE ac.currency_id = cer.currency_id AND
  cer.currency_exchange_id = (SELECT MAX(currency_exchange_id) FROM currencyExchange
    WHERE effective_date = (
      SELECT MAX(effective_date) FROM CurrencyExchange WHERE effective_date <= sysdate)) AND
  adserverutil.cross_status(
    adserverutil.cross_status(
      ag.adserver_status, ac.adserver_status),
    adserverutil.cross_status(
      ccg.adserver_status, c.status)) <> 'D'"

for (( i=1; i<=$iterations; i++ ))
do
  start=`date +%s`;
  $valgrind_prefix OracleStatementTest --db=$db -u $user -p $pass -q "$query" -t $delay
  stop=`date +%s`;
  passed=`expr $stop - $start`;
  if test $passed -ge $delay; then
    echo Test failed: too long data selection, `expr $passed / 60` minutes \
      `expr $passed % 60` seconds >&2
    exit 1
  fi
done

echo SUCCESS: All queries are executed faster than $delay seconds.
