#!/bin/bash

# files to add operator=
input_file_hpp=$1
input_file_cpp=$2

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

        #echo -e "after '${constructor_pattern}' \n add '${add_after}' \n"
    #else
        #echo -e "'${add_after}' уже существует в файле для класса ${target}.\n"
    fi
}

add_if_not_exists_for_2line() {
    local first_line="$1"
	local second_line="$2"
    local add_after="$3"
    local file="$4"
	local for_check="$5"
    local target="$6"

	#echo ${target}

    # Checking for presents in a file add_after
    if ! grep -q "${for_check}" "${file}"; then
        # writing to a file
		sed -i "/^${first_line}/ { n; /^${second_line}/ s/$/\n${add_after}/ }" ${file}

        #echo -e "after '${first_line}' '${second_line}' \n add '${add_after}' \n"
    #else
        #echo -e "'${add_after}' уже существует в файле для класса ${target}.\n"
    fi
}

#hpp process
# Инициализация пустого массива
classes=()
# Поиск классов в файле
find_classes "$input_file_hpp" classes

# Проверка, что найдены классы
if [ ${#classes[@]} -eq 0 ]; then
  #echo "Classes not found."
  exit 1
fi

# Iterating by classes arr - adding an ad to .hpp
for target in "${classes[@]}"; do
# Creating lines to insert
  first_line="${target} (const ${target} &);"
  add_after="\    ${target}& operator=(const ${target} &);"

# Adding a line if it doesn't already exist
  add_if_not_exists_for_1line "${first_line}" "${add_after}" "$input_file_hpp" "$target"
done

# Iterating by classes arr - adding an ad to .cpp
for target in "${classes[@]}"; do
# Creating lines to insert
    first_line="CORBACommons::${target}::${target} (void)"
    second_line="{}"
	add_after="\nCORBACommons::${target}\& CORBACommons::${target}::operator=(const CORBACommons::${target} \&other)\n{\n    CORBACommons::${target} temp(other);\n    swap(*this, temp);\n    return *this;\n}"
	for_check="CORBACommons::${target}& CORBACommons::${target}::operator=(const CORBACommons::${target} &other)"
    add_if_not_exists_for_2line "${first_line}" "${second_line}" "${add_after}" "$input_file_cpp" "${for_check}" "$target"
done
