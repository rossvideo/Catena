#!/bin/bash

# BSD 3-Clause License
#
# Copyright (c) 2015 - 2024 Intel Corporation (NEX Visual Solutions Poland)
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

function message() {
    local type=$1
    shift
    echo -e "$type: $*" >&2
}

function prompt() {
    local PomptHBlue='\e[38;05;33m';
    local PomptTBlue='\e[38;05;45m';
    message "${PomptHBlue}INFO${PomptTBlue}" "$*\e[m"
}

function error() {
    local ErrorHRed='\e[38;05;1m'
    local PomptTBlue='\e[38;05;45m';
    message "${ErrorHRed}ERROR${PomptTBlue}" "$*\e[m"
}

function warning() {
    local WarningHPurple='\e[38;05;61m';
    local PomptTBlue='\e[38;05;45m';
    message "${WarningHPurple}WARN${PomptTBlue}" "$*\e[m"
}

function print_logo()
{
    if [[ -z "$blue_code" ]]
    then
        local blue_code=( 26 27 20 19 20 20 21 04 27 26 32 12 33 06 39 38 44 45 )
    fi
    local IFS
	local logo_string
    local colorized_logo_string
    IFS=$'\n\t'
    logo_string="$(cat <<- EOF
	.-----------------------------------------------------------.
	|        *          .                    ..        .    *   |
	|                      .             .     . :  .   .    .  |
	|       .                         .   .  .  .   .           |
	|                                    . .  *:. . .           |
	|                             .  .   . .. .         .       |
	|                    .     . .  . ...    .    .             |
	|  .              .  .  . .    . .  . .                     |
	|                   .    .     . ...   ..   .       .       |
	|            .  .    . *.   . .                             |
	|                   :.  .           .                       |
	|            .   .    .    .                                |
	|        .  .  .    . ^                                     |
	|       .  .. :.    . |             .               .       |
	|.   ... .            |                                     |
	| :.  . .   *.        |     .               .               |
	| *.              We are here.                              |
	|   .               .             *.                        |
	.---------------------------------------ascii-author-unknown.
	=                                                           =
	=        88                                  88             =
	=        ""                ,d                88             =
	=        88                88                88             =
	=        88  8b,dPPYba,  MM88MMM  ,adPPYba,  88             =
	=        88  88P'   '"8a   88    a8P_____88  88             =
	=        88  88       88   88    8PP"""""""  88             =
	=        88  88       88   88,   "8b,   ,aa  88             =
	=        88  88       88   "Y888  '"Ybbd8"'  88             =
	=                                                           =
	=============================================================
	=    Intel Technology Poland 2024:                          =
	=            Network & Edge Group Visual Solutions Poland   =
	=-----------------------------------------------------------=
	=    Linkiewicz Milosz...... milosz.linkiewicz@intel.com    =
	=    Wysocki Piotr.......... piotr.wysocki@intel.com        =
	=============================================================
		EOF
    )"

    colorized_logo_string=""
    for (( i=0; i<${#logo_string}; i++ ))
    do
        colorized_logo_string+="\e[38;05;${blue_code[$(( (i-(i/64)*64)/4 ))]}m"
        colorized_logo_string+="${logo_string:$i:1}"
    done;
    colorized_logo_string+='\e[m\n'

    echo -e "$colorized_logo_string" >&2
}

function print_logo_sequence()
{
    local wait_between_frames="${1:-0}"
    local wait_cmd=""
    if [ ! "${wait_between_frames}" = "0" ]; then
        wait_cmd="sleep ${wait_between_frames}"
    fi

    blue_code_fixed=( 26 27 20 19 20 20 21 04 27 26 32 12 33 06 39 38 44 45 )
    size=${#blue_code_fixed[@]}
    for (( move=0; move<size; move++ ))
    do
    	blue_code=()
    	for (( i=move; i<size; i++ ))
    	do
    		blue_code+=("${blue_code_fixed[i]}")
    	done
    	for (( i=0; i<move; i++ ))
    	do
    		blue_code+=("${blue_code_fixed[i]}")
    	done
    	echo -en "\e[0;0H"
    	print_logo
        ${wait_cmd}
    done;
}

function print_logo_anim()
{
    local number_of_sequences="${1:-2}"
    local wait_between_frames="${2:-0}"
    clear
    for (( pt=0; pt<${number_of_sequences}; pt++ ))
    do
        print_logo_sequence ${wait_between_frames};
    done
}

function failure() {
    local _last_command_height=""
    local -n _lineno="${1:-LINENO}"
    local -n _bash_lineno="${2:-BASH_LINENO}"
    local _last_command="${3:-${BASH_COMMAND}}"
    local _code="${4:-0}"
    local -a _output_array=()
    _last_command_height="$(wc -l <<<"${_last_command}")"

    _output_array+=(
        '---'
        "lines_history: [${_lineno} ${_bash_lineno[*]}]"
        "function_trace: [${FUNCNAME[*]}]"
        "exit_code: ${_code}"
    )

    if [[ "${#BASH_SOURCE[@]}" -gt '1' ]]; then
        _output_array+=('source_trace:')
        for _item in "${BASH_SOURCE[@]}"; do
            _output_array+=("  - ${_item}")
        done
    else
        _output_array+=("source_trace: [${BASH_SOURCE[*]}]")
    fi

    if [[ "${_last_command_height}" -gt '1' ]]; then
        _output_array+=(
            'last_command: ->'
            "${_last_command}"
        )
    else
        _output_array+=("last_command: ${_last_command}")
    fi

    _output_array+=('---')
    printf '\e[38;05;1mERROR\e[38;05;45m: %s\e[m\n' "${_output_array[@]}" >&2
}

function get_user_input_def_yes() {
    get_user_input_confirm 1 "$@"
}

function get_user_input_def_no() {
    get_user_input_confirm 0 "$@"
}

function get_user_input_confirm() {
    local confirm_default="${1:-0}"
    shift
    local confirm=""
    local PomptHBlue='\e[38;05;33m';
    local PomptTBlue='\e[38;05;45m';

    prompt "$@"
    echo -en "${PomptHBlue}CHOOSE:  YES / NO   (Y/N) [default: " >&2
    [[ "$confirm_default" == "1" ]] && echo -en 'YES]: \e[m' >&2 || echo -en 'NO]: \e[m' >&2
    read confirm
    if [[ -z "$confirm" ]]
    then
        confirm="$confirm_default"
    else
        ( [[ $confirm == [yY] ]] || [[ $confirm == [yY][eE][sS] ]] ) && confirm="1" || confirm="0"
    fi
    echo "${confirm}"
}

function get_filename() {
    local path=$1
    echo ${path##*/}
}

function get_dirname() {
    local path=$1
    echo "${path%/*}/"
}

function get_extension() {
    local filename=$(get_filename $1)
    echo "${filename#*.}"
}

function get_basename() {
    local filename=$(get_filename $1)
    echo "${filename%%.*}"
}

# Takes ID as first param and full path to output file as second for creating benchmark specific output file
#  input: [path]/[file_base].[extension]
# output: [path]/[file_base][id].[extension]
function get_filepath_add_sufix() {
    local file_sufix="${1}"
    local file_path="${2}"
    local dir_path=$(get_dirname "${filepath}")
    local file_base=$(get_basename "${filepath}")
    local file_ext=$(get_extension "${filepath}")
    echo "${dir_path}${file_base}${sufix}.${file_ext}"
}

# File name is evaluated once at the begigning of sequence run
# %Y year, %m month, %d day, %H hour, %M minute, %S second, example:
#   - given path "/home/ubuntu/.hidden/plik_%Y%m%d_%H-%M-%S.scv"
#     will give  "/home/ubuntu/.hidden/plik_20220415_20-48-33.scv"
function get_filepath_eval_date() {
    local file_path=${1}
    echo $(date +"${file_path}")
}

# Takes INI file full path as input
# Return parsed file in form of list for evaluation:
#   declare -A label_name
#   export label_name["variable_name"]=variable_value
# For given:
#   [my_label]
#   api_endpoint="http://localhost:1234/"
# Returns:
#   declare -A my_label
#   export my_label["api_endpoint"]="http://localhost:1234/"
# Usage: eval $(parse_ini_file "my_ini_file.ini")

function parse_ini_file() {
    local input_filepath=$1

    local begin_omit_empty='BEGIN{FS="=";section="global";print"declare -A "section}/^$/{next}'
    local section_set='match($0,/^\[(.+)\]$/){gsub(/ /,"",$0);section=substr($1,RSTART+1,RLENGTH-2);print"declare -A "section;next}'
    local section_key_value='{gsub(/ /,"",$1);gsub(/^ */,"",$2);gsub(/ *$/,"",$2);print section"["$1"]="$2}'

    parsed_ini=$(cat "${input_filepath}" | awk "${begin_omit_empty}${section_set}${section_key_value}")
    echo "${parsed_ini}"
}

# Method for setting persistant environment variable
# Inputs
#   $1 - full_line - line to add in form of string
#   $2 - (optional) file_path - file in which the variable/line should be set [default is "/etc/environment"]
#   $3 - (optional) custom search string that line should start witch [default is `cut -d'=' -f1 $full_line`]
function add_apt_line()
{
  local safe_new_line=""
  local full_line="${1:-?}"
  local file_path="${2:-/etc/environment}"
  local search_string="${3:-$full_line}"
  local linenum=0
  local replace="FALSE"
  safe_new_line="$(printf "%s" "${full_line}" | sed -e 's/[\/&]/\\&/g')"

  if [ -e "${file_path}" ];
  then
    linenum="$(grep -n "${search_string}" < "${file_path}" | grep -Eo '^[^:]+' | tail -1)" && replace="TRUE"
    if [ "$replace" = "TRUE" ]
    then
      sudo sed -i "${linenum}s/.*/${safe_new_line}/" "${file_path}"
    else
      sudo bash -c "echo '${full_line}' >> \"${file_path}\""
    fi
  fi
  return 0
}

# Method for setting persistant environment variable
# Inputs
#   $1 - full_line - line to add in form of string 'NAME=VALUE'
#   $2 - (optional) file_path - file in which the variable/line should be set [default is "/etc/environment"]
#   $3 - (optional) custom search string that line should start witch [default is `cut -d'=' -f1 $full_line`]
function add_environment_line()
{
  local safe_new_line=""
  local line_number=""
  local full_line="${1}"
  local file_path="${2:-/etc/environment}"
  local search_string="${3}"

  if [ $# -lt 1 ]; then
    echo "Error in add_environment_line. Wrong number of arguments ($# < 1)." >&2
    return 1
  elif [ $# -eq 2 ]; then
    search_string="$(cut -d'=' -f1 <<< "${full_line}")"
  fi

  safe_new_line="$(sed -e 's/[\/&]/\\&/g' <<< "${full_line}")"
  line_number="$(grep -En "^${search_string}" < "${file_path}" | grep -Eo '^[^:]+' | tail -1 || echo '')"

  if [ -n "${line_number}" ]; then
    echo "sudo sed -i \"${line_number}s/.*/${safe_new_line}/\" \"${file_path}\""
  else
    echo "sudo bash -c \"echo '${full_line}' >> \"${file_path}\"\""
  fi
  return 0
}

function github_api_call() {
    url=$1
    shift
    GITHUB_API_URL=https://api.github.com
    INPUT_OWNER=$(echo "${url#"${GITHUB_API_URL}/repos/"}" | cut -f1 -d'/')
    INPUT_REPO=$(echo "${url#"${GITHUB_API_URL}/repos/"}" | cut -f2 -d'/')
    API_SUBPATH="${url#"${GITHUB_API_URL}/repos/${INPUT_OWNER}/${INPUT_REPO}/"}"
    if [ -z "${INPUT_GITHUB_TOKEN}" ]; then
        echo >&2 "Set the INPUT_GITHUB_TOKEN env variable first."
        return
    fi

    echo >&2 "GITHUB_API_URL=$GITHUB_API_URL"
    echo >&2 "INPUT_OWNER=$INPUT_OWNER"
    echo >&2 "INPUT_REPO=$INPUT_REPO"
    echo >&2 "API_SUBPATH=$API_SUBPATH"
    echo >&2 "curl --fail-with-body -sSL \"${GITHUB_API_URL}/repos/${INPUT_OWNER}/${INPUT_REPO}/${API_SUBPATH}\" -H \"Authorization: Bearer ${INPUT_GITHUB_TOKEN}\" -H 'Accept: application/vnd.github.v3+json' -H 'Content-Type: application/json' $*"

    if API_RESPONSE=$(curl --fail-with-body -sSL \
        "${GITHUB_API_URL}/repos/${INPUT_OWNER}/${INPUT_REPO}/${API_SUBPATH}" \
        -H "Authorization: Bearer ${INPUT_GITHUB_TOKEN}" \
        -H 'Accept: application/vnd.github.v3+json' \
        -H 'Content-Type: application/json' \
        "$@")
    then
        echo "${API_RESPONSE}"
    else
        echo >&2 "GitHub API call failed."
        echo >&2 "${API_RESPONSE}"
        return
    fi
}
