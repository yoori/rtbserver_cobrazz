#!/bin/bash

set -o pipefail

CACHE_PATH=''
TMP_CACHE_PATH=''
SSP_SOURCE_ID=''
SSP_GLOBAL_KEY=''
MIN_REQ_TIME=''

declare -A SSP_SOURCES
declare -A CHANNEL_GROUPS

while [ "$#" -gt "0" ]
do
  case $1 in
  --cache)
    shift
    CACHE_PATH=$1
    ;;
  --ssp-source)
    shift
    SSP_SOURCES[SINGLE_SOURCE]=$1
    ;;
  --ssp-source-*)
    ARG=$1
    KEY=${ARG:13}
    shift
    SSP_SOURCES[$KEY]=$1
    ;;
  --ssp-global-key)
    shift
    SSP_GLOBAL_KEY=$1
    ;;
  --channels)
    shift
    CHANNEL_GROUPS[SINGLE_SOURCE]=$1
    ;;
  --channels-*)
    ARG=$1
    KEY=${ARG:11}
    shift
    CHANNEL_GROUPS[$KEY]=$1
    ;;
  --tmp-cache)
    shift
    TMP_CACHE_PATH=$1
    ;;
  --min-req-time)
    shift
    MIN_REQ_TIME=$1
    ;;
  *) echo "Unknown parameter: $1" >&2
    exit 1
    ;;
  esac
  shift
done

for KEY in "${!SSP_SOURCES[@]}";
do
  if [ "${CHANNEL_GROUPS["$KEY"]}" == '' ]
  then
    echo "channels for ssp-source-$KEY isn't set" 1>&2
    exit 1
  fi
done

for KEY in "${!CHANNEL_GROUPS[@]}";
do
  if [ "${SSP_SOURCES["$KEY"]}" == '' ]
  then
    echo "source for channels-$KEY isn't set" 1>&2
    exit 1
  fi
done

if [ "$TMP_CACHE_PATH" == '' ]
then
  TMP_CACHE_PATH=$CACHE_PATH/../cache.for.extract
fi

if [ ! -e $CACHE_PATH ]
then
  echo "cache directory don't exists: $CACHE_PATH" 1>&2
  exit 1
fi

if [ -e $TMP_CACHE_PATH ]
then
  echo "temp cache directory already exists: $TMP_CACHE_PATH" 1>&2
  exit 1
fi

if [ "$MIN_REQ_TIME" == "" ] || [ "${#SSP_SOURCES[@]}" == "0" ]
then
  echo "These parameter must be defined and not empty: --min-req-time, --ssp-source-*" >&2
  exit 1
fi

# check that all required utils is available
UserExtractUtil --help 1>/dev/null 2>&1
[[ $? -ne 0 ]] && echo "UserExtractUtil unavailable" 1>&2 && exit 1

UserIdUtil --help 1>/dev/null 2>&1
[[ $? -ne 0 ]] && echo "UserIdUtil unavailable" 1>&2 && exit 1

ORIG_FILE_COUNT=0

pushd $CACHE_PATH/ExpressionMatcher/ >/dev/null
[[ $? -ne 0 ]] && echo "ExpressionMatcher cache not found ($CACHE_PATH/ExpressionMatcher/)" >&2 && exit 1

# prepare filters
ALL_CHANNELS='0'

for VALUE in "${CHANNEL_GROUPS[@]}"; do ALL_CHANNELS="$ALL_CHANNELS,$VALUE"; done

rm -f $TMP_CACHE_PATH/ExtractChannels.result

function cleanup {
  popd >/dev/null
  rm  -rf $TMP_CACHE_PATH >/dev/null 2>&1
}

#trap cleanup EXIT

mkdir -p $TMP_CACHE_PATH

# process chunks by one
EX=0

find . -maxdepth 1 -type d -name 'Chunk_*' | \
  while read CHUNK_DIR
  do
    pushd $CHUNK_DIR >/dev/null

    find . -type f -name 'Inventory_.0*' | \
      while read FILE
      do
        ORIG_FILE_COUNT=$((ORIG_FILE_COUNT+1))
        DIR=`dirname $FILE`
        mkdir -p $TMP_CACHE_PATH/ExpressionMatcher.Inventory/$CHUNK_DIR/$DIR
        [[ $? -ne 0 ]] && echo "can't create directory '$TMP_CACHE_PATH/ExpressionMatcher.Inventory/$DIR'" >&2 && exit 1
        ln $FILE $TMP_CACHE_PATH/ExpressionMatcher.Inventory/$CHUNK_DIR/$FILE 2>/dev/null # ignore if file removed concurrently
      done

    [[ $? -ne 0 ]] && EX=1
    [[ $EX -ne 0 ]] && echo "chunk hard linking interrupted" >&2 && EX=1 && exit 1

    FILE_COUNT=`find $TMP_CACHE_PATH/ExpressionMatcher.Inventory/$CHUNK_DIR -type f 2>/dev/null | wc -l`
    if [ $FILE_COUNT -eq 0 ] && [ $ORIG_FILE_COUNT -ne 0 ]
    then
      echo "can't create hard links for cache file, check that '$CACHE_PATH' and " \
        "'$TMP_CACHE_PATH' placed in one partition" >&2
      EX=1
      exit 1
    fi

    #COMMAND="( echo -e 'AAAAAAAAAAAAAAAAAAAAAA..\t$ALL_CHANNELS' ; echo -e 'BBBBBBBBBBBBBBBBBBBBBA..\t$ALL_CHANNELS' ; ) >$TMP_CACHE_PATH/UserExtractUtil.result"
    COMMAND="UserExtractUtil --path=$TMP_CACHE_PATH/ExpressionMatcher.Inventory/$CHUNK_DIR/Inventory/ --channels=$ALL_CHANNELS --min-req-time=$MIN_REQ_TIME >$TMP_CACHE_PATH/UserExtractUtil.result"
    eval "$COMMAND"
    [[ $? -ne 0 ]] && echo "failed running of : $COMMAND" >&2 && EX=1 && exit 1

    rm -r $TMP_CACHE_PATH/ExpressionMatcher.Inventory/$CHUNK_DIR

    # process $TMP_CACHE_PATH/UserExtractUtil.result
    for KEY in "${!SSP_SOURCES[@]}"
    do
      SSP_SOURCE_ID="${SSP_SOURCES["$KEY"]}"
      CHANNELS="${CHANNEL_GROUPS["$KEY"]}"

      cat $TMP_CACHE_PATH/UserExtractUtil.result | \
        perl -e 'my @ch_array = split(",", $ARGV[0]);
          my %ch;
          $ch{$_} = 1 for (@ch_array);

          while(<STDIN>)
          {
            chomp;
            my @parts = split("\t", $_);
            if(defined($parts[0]) && defined($parts[1]))
            {
              my $uid = $parts[0];
              my @uch = split(",", $parts[1]);
              my @res;
              foreach my $c(@uch)
              {
                exists $ch{$c} && push(@res, $c);
              }
              if (@res)
              {
                print $uid."\t".join(",", @res)."\n";
              }
            }
          }' "$CHANNELS" >$TMP_CACHE_PATH/ExtractChannels.orig.$SSP_SOURCE_ID

        cat $TMP_CACHE_PATH/ExtractChannels.orig.$SSP_SOURCE_ID | \
          UserIdUtil uid-to-sspuid --source=$SSP_SOURCE_ID --global-key=$SSP_GLOBAL_KEY --column=1 >>$TMP_CACHE_PATH/ExtractChannels.result.$SSP_SOURCE_ID

      [[ $? -ne 0 ]] && echo "failed running of : $COMMAND" >&2 && EX=1 && exit 1
    done

    [[ $EX -ne 0 ]] && echo "extracting interrupted ($?)" >&2 && exit 1

    popd >/dev/null
  done # chunk processing loop

[[ $? -ne 0 ]] && EX=1
[[ $EX -eq 0 ]] || exit 1

# output full result
for KEY in "${!SSP_SOURCES[@]}"
do
  SSP_SOURCE_ID="${SSP_SOURCES["$KEY"]}"
  echo $SSP_SOURCE_ID
  cat $TMP_CACHE_PATH/ExtractChannels.result.$SSP_SOURCE_ID
done

exit 0