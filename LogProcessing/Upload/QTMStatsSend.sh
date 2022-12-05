#!/bin/bash

ACCOUNT_ID=11258
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
\copy (SELECT campaign.campaign_id, adv_sdate, campaign.name campaign_name, campaigncreativegroup.name ccg_name, sum(imps) imps, sum(clicks) clicks, sum(pub_amount_adv) amount
FROM advertiserstatsdaily
  JOIN campaign ON(campaign.campaign_id = advertiserstatsdaily.campaign_id)
  JOIN campaigncreativegroup ON(campaigncreativegroup.ccg_id = advertiserstatsdaily.ccg_id)
WHERE adv_sdate > '$REQ_DATE' and adv_sdate < '$CUR_DATE' and (
  adv_account_id = $ACCOUNT_ID or adv_account_id IN (select account_id from Account where agency_account_id = $ACCOUNT_ID))
GROUP BY campaign.campaign_id, adv_sdate, campaign.name, campaigncreativegroup.name
ORDER BY 1,2,3,4) TO '$TMP_DIR/new/export.csv' WITH CSV
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
      perl CompareStats.pl "$OLD_FILE" "$NEW_FILE" >"$TMP_DIR/tmp/$F.$UUID"
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
    if curl -T "$TMP_DIR/upload/$F" 'ftp://ch1.qt.media' --user target:LTB7eW5+uEFTrg ; then
      rm -f "$TMP_DIR/upload/$F"
    else
      echo "Can't upload : $F" >&2
    fi
  done
done

