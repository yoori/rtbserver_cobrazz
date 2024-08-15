#!/bin/bash

# files to add operator=
input_file_hpp=$1
namespace_name_default=$2
exclude_string=$3
class_and_special_namespace=$4

#echo "input_file_hpp: $input_file_hpp"
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
#     echo -e "after '${first_line}' \n add '${add_after}' \n"
#   else
#     echo -e "'${add_after}' already presents int the file for class: ${target}.\n"
    fi
}

#hpp process
classes=()
# find classes in the file
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
  add_after="\    ${target}& operator=(const ${target} &) = default;"

# Adding a line if it doesn't already exist
  add_if_not_exists_for_1line "${first_line}" "${add_after}" "${input_file_hpp}" "${target}"
done
