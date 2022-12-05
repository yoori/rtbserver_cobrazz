#!/bin/bash

ACCOUNT_ID=11297
TMP_DIR="${ACCOUNT_ID}_upload"
DISTR_DIR="$TMP_DIR/new/distr"
mkdir -p "$DISTR_DIR"
mkdir -p "$TMP_DIR/prev/"
mkdir -p "$TMP_DIR/upload/"
mkdir -p "$TMP_DIR/uploaded/"
mkdir -p "$TMP_DIR/tmp/"

CUR_DATE=$(date "+%Y-%m-%d")
REQ_DATE=$(date "+%Y-%m-%d" -d "10 days ago")

SQL_REQ=$(cat <<EOF
\copy (SELECT campaign.campaign_id, country_sdate, campaign.name campaign_name, campaigncreativegroup.name ccg_name,
  pubaccount.name ssp_name, requeststatsdailycountry.country_code country_code, devicechannel.name device_name,
  sum(imps) imps, sum(clicks) clicks, sum(pub_amount_global * adv_ce_rate.rate) amount
FROM requeststatsdailycountry
  JOIN Account adv_account ON(adv_account.account_id = requeststatsdailycountry.adv_account_id)
  JOIN account AS pubaccount ON(pubaccount.account_id = requeststatsdailycountry.pub_account_id)
  JOIN campaign ON(campaign.campaign_id = requeststatsdailycountry.campaign_id)
  JOIN campaigncreativegroup ON(campaigncreativegroup.ccg_id = requeststatsdailycountry.ccg_id)
  JOIN channel AS devicechannel ON(devicechannel.channel_id = requeststatsdailycountry.device_channel_id)
  JOIN CurrencyExchange ce USING(currency_exchange_id)
  JOIN CurrencyExchangeRate adv_ce_rate ON(adv_ce_rate.currency_exchange_id = ce.currency_exchange_id and adv_ce_rate.currency_id = adv_account.currency_id)
WHERE country_sdate > '$REQ_DATE' and country_sdate < '$CUR_DATE' and (
  adv_account_id = $ACCOUNT_ID or adv_account_id IN (select account_id from Account where agency_account_id = $ACCOUNT_ID))
GROUP BY campaign.campaign_id, country_sdate, campaign.name, campaigncreativegroup.name, pubaccount.name, requeststatsdailycountry.country_code, devicechannel.name
ORDER BY 1,2,3,4,5,6,7) TO '$TMP_DIR/new/export.csv' WITH CSV
EOF
)

echo "$SQL_REQ"

PGPASSWORD=Q1oL6mm5hPTjnDQ psql -d stat -U ro -h postdb00 -t -c "$SQL_REQ"

source /opt/foros/server/etc/programmatic/adcluster/environment.sh

rm -rf "$DISTR_DIR"
mkdir "$DISTR_DIR"

cat "$TMP_DIR/new/export.csv" | \
  CsvFilter.pl --input='Input::Std(sep=>",")' --process='Output::Distrib(file=>"'"$DISTR_DIR"'/{1}-{2}")'


ls -1 -f "$DISTR_DIR/" | grep -v -E '^[.]' | while read F ; do
  echo "Process $F"
  NEW_FILE="$DISTR_DIR/$F"
  OLD_FILE="$TMP_DIR/uploaded/$F"
  # compare files
  NEW_FILE_HASH=$(md5sum "$NEW_FILE")

  if [ -f "$OLD_FILE" ] ; then
    echo "to compare with $OLD_FILE"
    OLD_FILE_HASH=$(md5sum "$OLD_FILE")

    if [[ "$NEW_FILE_HASH" != "$OLD_FILE_HASH" ]] ; then
      # files are different
      UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
      perl GGCompareStats.pl "$OLD_FILE" "$NEW_FILE" >"$TMP_DIR/tmp/$F.$UUID"
      if [ -s "$TMP_DIR/tmp/$F.$UUID" ]
      then
        mv "$TMP_DIR/tmp/$F.$UUID" "$TMP_DIR/upload/$F.$UUID"
      else
        rm "$TMP_DIR/tmp/$F.$UUID"
      fi
      cp "$NEW_FILE" "$OLD_FILE"
    fi
  else
    UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
    cp "$NEW_FILE" "$TMP_DIR/tmp/$F.$UUID"
    mv "$TMP_DIR/tmp/$F.$UUID" "$TMP_DIR/upload/$F.$UUID"
    cp "$NEW_FILE" "$OLD_FILE"
  fi

done

# upload loop
echo "To upload"

for i in $(seq 0 9) ; do
  ls -1 -f "$TMP_DIR/upload/" | grep -v -E '^[.]' | while read F ; do
    echo "To send : $F"
    if curl -T "$TMP_DIR/upload/$F" 'ftp://15.236.141.206' --user ftpuser:HZixPhlCHS ; then
      rm -f "$TMP_DIR/upload/$F"
    else
      echo "Can't upload : $F" >&2
    fi
  done
done

