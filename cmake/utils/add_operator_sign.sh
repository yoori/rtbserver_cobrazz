#!/bin/bash

# files to add operator=
input_file_hpp=$1
input_file_cpp=$2
namespace_name_default=$3
exclude_string=$4
class_and_special_namespace=$5

#echo "input_file_hpp: $input_file_hpp"
#echo "input_file_cpp: $input_file_cpp"
#echo "namespace_name_default: $namespace_name_default"
#echo "exclude_string: $exclude_string"
#echo "class_and_special_namespace: $class_and_special_namespace"

# Split the string into an array by spaces(this is need because cmake pass params as one string with spaces)
IFS=' ' read -r -a exclude_classes <<< "$exclude_string"

##to be sure that array was filled
#echo "exclude_classes:"
#for i in "${exclude_classes[@]}"; do
#    echo "$i"
#done

#Key value array - key for classname and value particular namespace
#format example "box1=apple, box2=apple, box3=pear"
declare -A class_and_special_namespace_map
# Split the string into key-value pairs
IFS=', ' read -ra pairs <<< "$class_and_special_namespace"

#fill the map - class_and_special_namespace_map
for pair in "${pairs[@]}"; do
  IFS='=' read -ra kv <<< "$pair"
  key="${kv[0]}"
  value="${kv[1]}"
  class_and_special_namespace_map["$key"]="$value"
done

##check that map was filled
#echo "class_and_special_namespace_map:"
#for key in "${!class_and_special_namespace_map[@]}"; do
#    echo "$key -> ${class_and_special_namespace_map[$key]}"
#done

#get value from map
#usage example: value_box1=$(get_value "box1")
get_value() {
  local key="$1"
  echo "${class_and_special_namespace_map[$key]}"
}

# find classes declaration
find_classes() {
    local file="$1"
    local -n classes_ref="$2"

    while IFS= read -r line; do
        # Extracting the class name from a string
        class_name=$(echo "$line" | grep -oP 'class\s+\K[A-Za-z]+')
        if [ -n "$class_name" ]; then
            classes_ref+=("$class_name")
        fi
    done < <(grep -P 'class [A-Za-z]+;' "$file")
}

# Function to remove excluded classes from the classes array
remove_excluded_classes() {
    local -n classes_ref="$1"
    local -n exclude_ref="$2"
    local new_classes=()


    for class in "${classes_ref[@]}"; do
        local exclude=false
        for exclude_class in "${exclude_ref[@]}"; do
            if [[ "$class" == "$exclude_class" ]]; then
                exclude=true
                break
            fi
        done
        if [ "$exclude" = false ]; then
            new_classes+=("$class")
        fi
    done

    classes_ref=("${new_classes[@]}")
}

add_if_not_exists_for_1line() {
    local first_line="$1"
    local add_after="$2"
    local file="$3"
    local target="$4"

    #echo ${target}

    # Checking for presents in a file add_after
    if ! grep -q "${add_after}" "${file}"; then
        # writing to a file
		sed -i "/${first_line}/a ${add_after}" "$file"

#        echo -e "after '${first_line}' \n add '${add_after}' \n"
#    else
#        echo -e "'${add_after}' уже существует в файле для класса ${target}.\n"
    fi
}

add_if_not_exists_for_2lines() {
    local first_line="$1"
    local second_line="$2"
    local add_after="$3"
    local file="$4"
    local for_check="$5"
    local target="$6"

#	echo ${target}

    # Checking for presents in a file add_after
    if ! grep -q "${for_check}" "${file}"; then
        # writing to a file
		sed -i "/^${first_line}/ { n; /^${second_line}/ s/$/\n${add_after}/ }" ${file}

#        echo -e "after '${first_line}' '${second_line}' \n add '${add_after}' \n"
#    else
#        echo -e "'${add_after}' уже существует в файле для класса ${target}.\n"
    fi
}

#hpp process
# Инициализация пустого массива
classes=()
# Поиск классов в файле
find_classes "$input_file_hpp" classes
#echo "classes:"
#for i in "${classes[@]}"; do
#    echo "$i"
#done

# Remove excluded classes from the classes array
remove_excluded_classes classes exclude_classes
#echo "classes after remove:"
#for i in "${classes[@]}"; do
#    echo "$i"
#done

# check that classes were found
if [ ${#classes[@]} -eq 0 ]; then
  echo "Classes not found."
  exit 1
fi

# Iterating by classes arr - adding an ad to .hpp
for target in "${classes[@]}"; do
# Creating lines to insert
  first_line="${target} (const ${target} &);"
  add_after="\    ${target}& operator=(const ${target} &);"

# Adding a line if it doesn't already exist
  add_if_not_exists_for_1line "${first_line}" "${add_after}" "${input_file_hpp}" "${target}"
done

# Iterating by classes arr - adding an ad to .cpp
for target in "${classes[@]}"; do
# Creating lines to insert
  value_map=$(get_value ${target})
  # check that it was found
  if [ -z "$value_map" ]; then
      namespace_to_use="${namespace_name_default}"
  else
      namespace_to_use="$value_map"
  fi
#  echo "target: $target, namespace_to_use: $namespace_to_use"

  first_line="${namespace_to_use}::${target}::${target} (void)"
  second_line="{}"
	add_after="${namespace_to_use}::${target}\& ${namespace_to_use}::${target}::operator=(const ${namespace_to_use}::${target} \&other)\n{\n    ${namespace_to_use}::${target} temp(other);\n    std::swap(*this, temp);\n    return *this;\n}"
	for_check="${namespace_to_use}::${target}& ${namespace_to_use}::${target}::operator=(const ${namespace_to_use}::${target} &other)"
  add_if_not_exists_for_2lines "${first_line}" "${second_line}" "${add_after}" "${input_file_cpp}" "${for_check}" "{$target}"
done