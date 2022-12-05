#!/bin/sh

echo XST transformation test started.

data_path="${1}"
valgrind_prefix="${2}"

if [ -z "$TEST_TMP_DIR" ]; then
  TEST_TMP_DIR=../../../../build/test/tmp
  mkdir -p $TEST_TMP_DIR
fi

if [ -z "$data_path" ]; then
  data_path=`pwd`/Data
fi

# These transformations should be passed
for xml_file in `tar -tf $data_path/Pass_xmls.tar`
do
  xml_file_name=`basename $xml_file .bz2`
  for file in $data_path/pass_*.xsl
  do
    file_name=`basename $file .xsl`
    tar -Oxf $data_path/Pass_xmls.tar $xml_file | \
     bzip2 -cd | \
       $valgrind_prefix XslTransformAdmin $data_path/$file_name.xsl /dev/stdin \
         > $TEST_TMP_DIR/$file_name.xsl_$xml_file_name.out
    if test $? -ne 0; then
      echo Valid transformation FAILED. Params: $file_name.xsl $xml_file_name >&2
    fi
# Get standards results from archive and compare with done transformation.
    tar -Oxf $data_path/Expectations.tar expected_$file_name.xsl_$xml_file_name.out.bz2 | \
     bzip2 -cd | \
      diff -a -B /dev/stdin $TEST_TMP_DIR/$file_name.xsl_$xml_file_name.out >&2
  done
done

#fails test
# Should NOT pass

for file in $data_path/error_*.xsl
do
  file_name=`basename $file .xsl`
  $valgrind_prefix XslTransformAdmin $data_path/$file_name.xsl $data_path/pass_test.xml 2>/dev/null
  if test $? -eq 0; then
    echo Invalid transformation PASSED. File name $file_name.xsl >&2
  fi
done

# Tests XslTransformer constructor with base URI.

tar -Oxf $data_path/Pass_xmls.tar pass_simple.xml.bz2 | \
     bzip2 -cd > $TEST_TMP_DIR/pass_simple.xml

$valgrind_prefix XslTransformAdmin -b $data_path/RelativeXSL/ \
  $data_path/InputXSL/main_simple.xsl $TEST_TMP_DIR/pass_simple.xml \
    > $TEST_TMP_DIR/report.out

$valgrind_prefix XslTransformAdmin -b $data_path/RelativeXSL2/ \
  $data_path/InputXSL/main_simple.xsl $TEST_TMP_DIR/pass_simple.xml \
    >> $TEST_TMP_DIR/report.out

# should ignore base URI parameter because absolute URI into main_absolute_simple.xsl
$valgrind_prefix XslTransformAdmin -b $data_path/RelativeXSL2/ \
  $data_path/InputXSL/main_absolute_simple.xsl $TEST_TMP_DIR/pass_simple.xml \
    >> $TEST_TMP_DIR/report.out

# Check correctness of output
diff -a -B $TEST_TMP_DIR/report.out $data_path/expectation_base_uri_test.out >&2

# Test XSL parameters setting

$valgrind_prefix XslTransformAdmin -b ./ -p test_xsl_parameter1:test_xsl_parameter3:test_xsl_parameter4 \
  -v SetValue1:SetValue3 $data_path/test_xsl_parameters.xsl $TEST_TMP_DIR/pass_simple.xml \
  > $TEST_TMP_DIR/report_parameters.out

# Check correctness of output
diff -a -B $TEST_TMP_DIR/report_parameters.out $data_path/expectation_xsl_parameters_test.out >&2

echo All tests completed.
