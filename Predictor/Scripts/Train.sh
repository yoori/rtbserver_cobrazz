#!/bin/bash

# run example: Train.sh 200000 Merged.csv Result_g8_c8 8 8 /home/jurij_kuznecov/Predictor/

ulimit -c 100000

TRUST_MIN=$1
IN_FILE=$2
IN_ROOT=$3
GLOBAL_TOP_FUSE=$4  # number of top features that will be used for train global model
CAMPAIGN_TOP_FUSE=$5  # number of top features that will be used for train campaign model
CONFIG_ROOT=$6 # config root
C_BEST_FEATURES_ROWS=$7 # best features search rows
C_TRAIN_ROWS=$8 # train rows
TRAIN_TYPE=$9

CAMPAIGN_GLOBAL_TOP_FUSE=5

[[ -z "$C_BEST_FEATURES_ROWS" ]] && C_BEST_FEATURES_ROWS=5000000
[[ -z "$C_TRAIN_ROWS" ]] && C_TRAIN_ROWS=30000000

# 1: Target Action
# 2: Target Action Timestamp
# 3: Clicked
# 4: Click Timestamp
# 5: Timestamp => wd,hour
# 6: Request ID
# 7: Global Request ID
# 8: Device => device
# 9: IP Address
# 10: HID
# 11: UID
# 12: URL
# 13: Tag ID => tag
# 14: External Tag ID
# 15: Campaign Creative ID
# 16: Geo Channels => geoch
# 17: User Channels => userch
# 18: Impression Channels
# 19: Bid Price
# 20: Bid Floor
# 21: Algorithm ID
# 22: Size ID => sizeid
# 23: Colo ID
# 24: Predicted CTR
# 25: Campaign Frequency => campaign_freq,campaign_freq_log
# 26: CR Algorithm ID
# 27: Predicted CR
# 28: Impression Timestamp
# 29: Impression UID
# 30: Win Price

XML='--xml'
CSV_HEAD='#Target Action,#Target Action Timestamp,Label,#Click Timestamp,Timestamp,#Request ID,#Global Request ID,Device,#IP Address,#HID,#UID,#URL,Tag,ETag,CCID,GeoCh,UserCh,#ImpCh,#Bid Price,#Bid Floor,#Algorithm ID,SizeID,Colo,#Predicted CTR,Campaign_Freq,#CR Algorithm ID,#Predicted CR,#Impression Timestamp,#Impression UID,#Win Price'
LOCAL_CSV_HEAD="$CSV_HEAD,#Campaign"

if [ "$TRAIN_TYPE" == "conv" ]
then
  CSV_HEAD='Label,#Target Action Timestamp,#Clicked,#Click Timestamp,Timestamp,#Request ID,#Global Request ID,Device,#IP Address,#HID,#UID,#URL,Tag,ETag,CCID,GeoCh,UserCh,#ImpCh,#Bid Price,#Bid Floor,#Algorithm ID,SizeID,Colo,#Predicted CTR,Campaign_Freq,#CR Algorithm ID,#Predicted CR,#Impression Timestamp,#Impression UID,#Win Price,Campaign'
  LOCAL_CSV_HEAD="$CSV_HEAD,#Campaign"
fi

GLOBAL_TRAIN_ROWS=$C_TRAIN_ROWS
GLOBAL_BEST_FEATURES_SEARCH_ROWS=$C_BEST_FEATURES_ROWS
CAMPAIGN_TRAIN_ROWS=$C_TRAIN_ROWS
CAMPAIGN_BEST_FEATURES_SEARCH_ROWS=$C_BEST_FEATURES_ROWS

#source /home/jurij_kuznecov/projects/foros/run/trunk/etc/adserver/adcluster/environment.sh
#export PERL5LIB=$PERL5LIB:/home/jurij_kuznecov/Predictor/Utils

CONFIG_TEMPL_ROOT=$CONFIG_ROOT/CTRConfigTempl/
DICT_ROOT=$CONFIG_ROOT/Dictionaries/

RESULT_CONFIG=$IN_ROOT/CTRConfig/

# start
mkdir -p $RESULT_CONFIG

mkdir -p $IN_ROOT/GlobalSVM
mkdir -p $IN_ROOT/GlobalFeatures

mkdir -p $IN_ROOT/CampaignCSV
mkdir -p $IN_ROOT/CampaignSVM
mkdir -p $IN_ROOT/CampaignChannelsSVM
mkdir -p $IN_ROOT/CampaignFilteredSVM
mkdir -p $IN_ROOT/CampaignBestFeatures
mkdir -p $IN_ROOT/CampaignBestChannelFeatures
mkdir -p $IN_ROOT/CampaignBestFilteredFeatures
mkdir -p $IN_ROOT/CampaignTrees
mkdir -p $IN_ROOT/CampaignChannelTrees
mkdir -p $IN_ROOT/CampaignChannelTreesDiv
mkdir -p $IN_ROOT/XGBModels
mkdir -p $IN_ROOT/www
touch $IN_ROOT/Global.features

echo '<html><body>' >$IN_ROOT/www/index.html

#
if [ "$TRAIN_TYPE" != "conv" ]
then
  cat $CONFIG_TEMPL_ROOT/model_config.json.begin.t >$RESULT_CONFIG/config.json

  # train global algorithm
  cat $IN_FILE | tail -n $GLOBAL_TRAIN_ROWS >$IN_ROOT/PartGlobal.csv
  TRAIN_CSV=$IN_ROOT/Global-Train.csv
  TEST_CSV=$IN_ROOT/Global-Test.csv

  # divide to test & train
  cat $IN_ROOT/PartGlobal.csv | \
    perl -e 'open(TEST_FILE, ">", "'$TEST_CSV'") ; open(TRAIN_FILE, ">", "'$TRAIN_CSV'") ; while (<STDIN>){ if(rand() <= 0.1) { print TEST_FILE $_ } else { print TRAIN_FILE $_ }; }'

  UNFILTERED_TRAIN_SVM=$IN_ROOT/Global-UnfilteredTrain.libsvm

  cat $TRAIN_CSV | \
    CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
    "$CSV_HEAD" \
    --dictionary=$IN_ROOT/Merged.dictionary \
    --cc-to-ccg=$DICT_ROOT/cc_to_ccg.csv \
    --cc-to-campaign=$DICT_ROOT/cc_to_campaign.csv \
    --tag-to-publisher=$DICT_ROOT/tag_to_publisher.csv \
    --name-dict=$DICT_ROOT/names.csv \
    >$UNFILTERED_TRAIN_SVM

  ALL_TOP_FEATURES=$IN_ROOT/Merged.best-features
  cat $UNFILTERED_TRAIN_SVM | tail -n$GLOBAL_BEST_FEATURES_SEARCH_ROWS | \
    PrFeatureFilter best-tree-features -d 30 -me 10 --dump-tree=$IN_ROOT/GlobalTree.xml $XML \
      --dict=$IN_ROOT/Merged.dictionary \
      >$ALL_TOP_FEATURES

  TOP_FEATURES=$IN_ROOT/Global.features

  cat $ALL_TOP_FEATURES | tr ',' '\n' | head -n $GLOBAL_TOP_FUSE | \
    tr '\n' ',' | sed 's/,$//' >$TOP_FEATURES

  cat $TOP_FEATURES | tr ',' '\n' | \
    sed -r 's/^(.*)$/\1,\1/' >$RESULT_CONFIG/global.features

  TRAIN_SVM=$IN_ROOT/Global-Train.libsvm
  TEST_SVM=$IN_ROOT/Global-Test.libsvm

  cat $TRAIN_CSV | \
    CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
      "$CSV_HEAD" \
      | PrFeatureFilter filter-svm -f `cat $TOP_FEATURES` >$TRAIN_SVM
  cat $TEST_CSV | \
    CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
      "$CSV_HEAD" \
      | PrFeatureFilter filter-svm -f `cat $TOP_FEATURES` >$TEST_SVM

  XGBoostTrain.py $RESULT_CONFIG/global.xgb $TRAIN_SVM \
    $TEST_SVM -n200 -d19 --eta 0.1 >$IN_ROOT/tmp

  XGBoostImportance.py $IN_ROOT/www/global_importance.png $RESULT_CONFIG/global.xgb \
    $IN_ROOT/Merged.dictionary

  echo "<center>GLOBAL IMPORTANCE GRAPH</center>" >>$IN_ROOT/www/index.html
  echo '</br><img height="600" width="100%" src="global_importance.png"/>' >>$IN_ROOT/www/index.html
  cp $IN_ROOT/GlobalTree.xml $IN_ROOT/www/
  echo '<a href="GlobalTree.xml">Global Tree Link</a>' >>$IN_ROOT/www/index.html

  rm -f $RESULT_CONFIG/xgb_global.blacklist
  touch $RESULT_CONFIG/xgb_global.blacklist

  echo "Global train: " >$RESULT_CONFIG/notes
  cat $IN_ROOT/tmp | tail -n2 >>$RESULT_CONFIG/notes
else
  cat $CONFIG_TEMPL_ROOT/conv_config.json.begin.t >$RESULT_CONFIG/config.json
fi

#
# divide by campaigns and train campaign algorithms
#
cat $IN_FILE | CsvFilter.pl --input=Input::Std \
  --process='Process::Columns(field=>"1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,15")' \
  --process='Process::NumFilter(field=>"31")' \
  --process='Process::ReplaceByFile(field=>"31",file=>"'$DICT_ROOT'/cc_to_campaign.csv")' \
  --process='Output::Distrib(file=>"'$IN_ROOT'/CampaignCSV/{31}.csv")'

ls -1 $IN_ROOT/CampaignCSV/ | \
while read LINE ; do
  CAMPAIGN_ID=`echo "$LINE" | cut -d'.' -f1`
  ROWS=`cat $IN_ROOT/CampaignCSV/$LINE | head -n$CAMPAIGN_TRAIN_ROWS | wc -l`
  echo "$ROWS $CAMPAIGN_ID" >>$IN_ROOT/sort_campaigns
done

cat $IN_ROOT/sort_campaigns | sort -rn | \
while read LINE ; do
  ROWS=`echo "$LINE" | cut -d' ' -f1`
  CAMPAIGN_ID=`echo "$LINE" | cut -d' ' -f2`
  CAMPAIGN_NAME=`cat $DICT_ROOT/names.csv | grep "campaign:$CAMPAIGN_ID," | sed "s/campaign:$CAMPAIGN_ID,//"`

  if [ "$ROWS" -gt $TRUST_MIN ]
  then
    # convert campaign imps to libsvm
    cat $IN_ROOT/CampaignCSV/$CAMPAIGN_ID.csv | tail -n$ROWS >$IN_ROOT/CampaignCSV/Part$CAMPAIGN_ID.csv
    TRAIN_CSV=$IN_ROOT/CampaignCSV/$CAMPAIGN_ID-Train.csv
    TEST_CSV=$IN_ROOT/CampaignCSV/$CAMPAIGN_ID-Test.csv
    cat $IN_ROOT/CampaignCSV/Part$CAMPAIGN_ID.csv | \
      perl -e 'open(TEST_FILE, ">", "'$TEST_CSV'") ; open(TRAIN_FILE, ">", "'$TRAIN_CSV'") ; while (<STDIN>){ if(rand() <= 0.1) { print TEST_FILE $_ } else { print TRAIN_FILE $_ }; }'

    # divide to test & train
    UNFILTERED_TRAIN_SVM=$IN_ROOT/CampaignSVM/$CAMPAIGN_ID-Train.libsvm
    cat $TRAIN_CSV | \
      CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
        "$LOCAL_CSV_HEAD" \
          --dictionary=$IN_ROOT/CampaignSVM/$CAMPAIGN_ID.dictionary \
          --cc-to-ccg=$DICT_ROOT/cc_to_ccg.csv \
          --cc-to-campaign=$DICT_ROOT/cc_to_campaign.csv \
          --tag-to-publisher=$DICT_ROOT/tag_to_publisher.csv \
          --name-dict=$DICT_ROOT/names.csv \
          >$UNFILTERED_TRAIN_SVM

    # get all features in priority desc order
    ALL_TOP_FEATURES=$IN_ROOT/CampaignBestFeatures/$CAMPAIGN_ID

    cat $UNFILTERED_TRAIN_SVM | tail -n$CAMPAIGN_BEST_FEATURES_SEARCH_ROWS | \
      PrFeatureFilter best-tree-features -d 20 -me 10 \
        --dict=$IN_ROOT/CampaignSVM/$CAMPAIGN_ID.dictionary \
        --dump-tree=$IN_ROOT/CampaignTrees/$CAMPAIGN_ID.xml \
        $XML \
      >$ALL_TOP_FEATURES

    # get top features
    TOP_FEATURES=$IN_ROOT/CampaignBestFilteredFeatures/$CAMPAIGN_ID
    ( cat $ALL_TOP_FEATURES | tr ',' '\n' | head -n $CAMPAIGN_TOP_FUSE ; \
      cat $IN_ROOT/Global.features | tr ',' '\n' | head -n $CAMPAIGN_GLOBAL_TOP_FUSE ) | \
      tr '\n' ',' | sed 's/,$//' >$TOP_FEATURES

    # filter
    TRAIN_SVM=$IN_ROOT/CampaignFilteredSVM/$CAMPAIGN_ID-Train.libsvm
    TEST_SVM=$IN_ROOT/CampaignFilteredSVM/$CAMPAIGN_ID-Test.libsvm

    cat $TRAIN_CSV | \
      CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
        "$LOCAL_CSV_HEAD" \
        | PrFeatureFilter filter-svm -f `cat $TOP_FEATURES` >$TRAIN_SVM
    cat $TEST_CSV | \
      CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
        "$LOCAL_CSV_HEAD" \
        | PrFeatureFilter filter-svm -f `cat $TOP_FEATURES` >$TEST_SVM

    # train
    echo "`date '+%F %T'` : TRAIN FOR CAMPAIGN #"$CAMPAIGN_ID"($CAMPAIGN_NAME)"

    XGBoostTrain.py \
      $IN_ROOT/XGBModels/$CAMPAIGN_ID.xgb \
      $TRAIN_SVM \
      $TEST_SVM -n200 -d19 --eta 0.1 >$IN_ROOT/tmp

    # check logloss
    cat "$TEST_CSV" | CTRGenerator generate-xgb-ctr $IN_ROOT/XGBModels/$CAMPAIGN_ID.xgb $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
      '#Target Action,#Target Action Timestamp,Label,#Click Timestamp,Timestamp,#Request ID,#Global Request ID,Device,#IP Address,#HID,#UID,#URL,Tag,ETag,CCID,GeoCh,UserCh,#ImpCh,#Bid Price,#Bid Floor,#Algorithm ID,SizeID,Colo,#Predicted CTR,Campaign_Freq,#CR Algorithm ID,#Predicted CR,#Impression Timestamp,#Impression UID,#Win Price' \
      --cc-to-ccg=$DICT_ROOT/cc_to_ccg.csv \
      --cc-to-campaign=$DICT_ROOT/cc_to_campaign.csv \
      --tag-to-publisher=$DICT_ROOT/tag_to_publisher.csv >$IN_ROOT/ccheck.ctrs

    LOCAL_CHECK_LOGLOSS=`cat "$TEST_CSV" | CsvFilter.pl --input=Input::Std --process='Process::Columns(field=>"3")' \
      --process='Process::MergeByRows(file=>"'$IN_ROOT'/ccheck.ctrs")' \
      --process='Process::LogLoss(field=>2)' | grep LogLoss | sed -r 's/^.*: ([0-9.]+)[(].*$/\1/'`

    if [ "$TRAIN_TYPE" != "conv" ]
    then
      cat "$TEST_CSV" | CTRGenerator generate-xgb-ctr $RESULT_CONFIG/global.xgb $CONFIG_TEMPL_ROOT/UseCTRConf.xml \
        '#Target Action,#Target Action Timestamp,Label,#Click Timestamp,Timestamp,#Request ID,#Global Request ID,Device,#IP Address,#HID,#UID,#URL,Tag,ETag,CCID,GeoCh,UserCh,#ImpCh,#Bid Price,#Bid Floor,#Algorithm ID,SizeID,Colo,#Predicted CTR,Campaign_Freq,#CR Algorithm ID,#Predicted CR,#Impression Timestamp,#Impression UID,#Win Price' \
        --cc-to-ccg=$DICT_ROOT/cc_to_ccg.csv \
        --cc-to-campaign=$DICT_ROOT/cc_to_campaign.csv \
        --tag-to-publisher=$DICT_ROOT/tag_to_publisher.csv >$IN_ROOT/gcheck.ctrs

      GLOBAL_CHECK_LOGLOSS=`cat "$TEST_CSV" | CsvFilter.pl --input=Input::Std --process='Process::Columns(field=>"3")' \
        --process='Process::MergeByRows(file=>"'$IN_ROOT'/gcheck.ctrs")' \
        --process='Process::LogLoss(field=>2)' | grep LogLoss | sed -r 's/^.*: ([0-9.]+)[(].*$/\1/'`

      echo "`date '+%F %T'` : Campaign #"$CAMPAIGN_ID": global loss = $GLOBAL_CHECK_LOGLOSS, campaign loss = $LOCAL_CHECK_LOGLOSS, rows=$ROWS"
      LOSS_COMPARE=`echo "$LOCAL_CHECK_LOGLOSS < $GLOBAL_CHECK_LOGLOSS" | bc -l`
    else
      echo "`date '+%F %T'` : Campaign #"$CAMPAIGN_ID": campaign loss = $LOCAL_CHECK_LOGLOSS, rows=$ROWS"
      LOSS_COMPARE=1
    fi

    if [ "$LOSS_COMPARE" -ge "1" ]
    then
      echo "Campaign #"$CAMPAIGN_ID" train: " >>$RESULT_CONFIG/notes

      cat $ALL_TOP_FEATURES | tr ',' '\n' | head -n $CAMPAIGN_TOP_FUSE | \
        sed -r 's/^(.*)$/\1,\1/' >$RESULT_CONFIG/campaign_$CAMPAIGN_ID.features

      cat $IN_ROOT/tmp | tail -n1 >>$RESULT_CONFIG/notes

      cp $IN_ROOT/XGBModels/$CAMPAIGN_ID.xgb $RESULT_CONFIG/campaign_$CAMPAIGN_ID.xgb

      cat $CONFIG_TEMPL_ROOT/model_config.json.alg.t | sed 's/$ID/'$CAMPAIGN_ID'/g' >>$RESULT_CONFIG/config.json
      echo "$CAMPAIGN_ID" >>$RESULT_CONFIG/xgb_global.blacklist
      echo "$CAMPAIGN_ID" >$RESULT_CONFIG/campaign_$CAMPAIGN_ID.whitelist
    fi

    XGBoostImportance.py $IN_ROOT/www/campaign_${CAMPAIGN_ID}_importance.png $IN_ROOT/XGBModels/$CAMPAIGN_ID.xgb \
      $IN_ROOT/CampaignSVM/$CAMPAIGN_ID.dictionary

    # fill channels
    FULL_CHANNEL_SVM=$IN_ROOT/CampaignChannelsSVM/$CAMPAIGN_ID.libsvm

    # convert campaign imps to libsvm
    cat $IN_ROOT/CampaignCSV/$CAMPAIGN_ID.csv | \
      CTRGenerator generate-svm $CONFIG_TEMPL_ROOT/UseOnlyChannelsCTRConf.xml \
        --dictionary=$IN_ROOT/CampaignChannelsSVM/$CAMPAIGN_ID.dictionary \
        "$LOCAL_CSV_HEAD" \
        >$FULL_CHANNEL_SVM

    mkdir $IN_ROOT/CampaignChannelTreesDiv/$CAMPAIGN_ID

    cat $FULL_CHANNEL_SVM | tail -n$CAMPAIGN_BEST_FEATURES_SEARCH_ROWS | \
      PrFeatureFilter best-tree-features -d 20 -me 10 \
        --dict=$IN_ROOT/CampaignChannelsSVM/$CAMPAIGN_ID.dictionary \
        --dump-tree=$IN_ROOT/CampaignChannelTrees/$CAMPAIGN_ID.xml \
        --trees-directory=$IN_ROOT/CampaignChannelTreesDiv/$CAMPAIGN_ID/ \
        --min-cover=1000 \
        $XML \
        >$IN_ROOT/CampaignBestChannelFeatures/$CAMPAIGN_ID

    echo "</br><center>CAMPAIGN #"$CAMPAIGN_ID"($CAMPAIGN_NAME)</center>" >>$IN_ROOT/www/index.html
    echo "<center>imps = $ROWS</center>" >>$IN_ROOT/www/index.html
    #echo "<center>actions = $ROWS</center>" >>$IN_ROOT/www/index.html
    echo '<a href="'$CAMPAIGN_ID'.xml">Feature Tree</a>' >>$IN_ROOT/www/index.html
    echo '<a href="'$CAMPAIGN_ID'_channel.xml">Channel Tree</a>' >>$IN_ROOT/www/index.html
    echo "<center>feature importance graph</center>" >>$IN_ROOT/www/index.html
    echo '</br><img height="600" width="100%" src="campaign_'$CAMPAIGN_ID'_importance.png"/>' >>$IN_ROOT/www/index.html
    cp $IN_ROOT/CampaignTrees/$CAMPAIGN_ID.xml $IN_ROOT/www/
    cp $IN_ROOT/CampaignChannelTrees/$CAMPAIGN_ID.xml $IN_ROOT/www/${CAMPAIGN_ID}_channel.xml

  else
    echo "`date '+%F %T'` : CAMPAIGN #"$CAMPAIGN_ID" WILL USE GLOBAL MODEL (low imps value)" >>$IN_ROOT/www/index.html
  fi
done

echo '</body></html>' >>$IN_ROOT/www/index.html

cat $CONFIG_TEMPL_ROOT/model_config.json.end.t >>$RESULT_CONFIG/config.json
