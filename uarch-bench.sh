#! /usr/bin/env bash
# launches the uarch-bench process after trying to disable turbo
# Some parts based on http://notepad2.blogspot.com/2014/11/a-script-to-turn-off-intel-cpu-turbo.html

set -e

export VENDOR_ID=$(lscpu | grep 'Vendor ID' | egrep -o '[^ ]*$')
export MODEL_NAME=$(lscpu | grep 'Model name' | sed -n 's/Model name:\s*\(.*\)$/\1/p')
export SCALING_DRIVER=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_driver)
export SCALING_GOVERNOR=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
export NO_TURBO_FILE="/sys/devices/system/cpu/intel_pstate/no_turbo"

function check_msrtools_installed {
	if [[ -z $(which rdmsr) ]]; then
		echo "msr-tools is not installed. Run 'sudo apt-get install msr-tools' to install it." >&2
		exit 1
	fi
}

function get_core_turbo_intel {
	echo $(sudo rdmsr -p${core} 0x1a0 -f 38:38)
}

function get_turbo_state_string {
	local turbo_state
	for core in $cores; do
		turbo_state+="$(get_core_turbo_intel $core) "core
	done
	echo $turbo_state
}

original_state=$(get_turbo_state_string)
original_no_turbo=0

function restore_no_turbo {
	if [ -e $NO_TURBO_FILE ]; then

		NO_TURBO=$original_no_turbo

		# restore the no_turbo if we changed it
		if [[ $NO_TURBO == 0 ]]; then
			echo -e "Reverting no_turbo to $NO_TURBO"
			sudo sh -c "echo $NO_TURBO > /sys/devices/system/cpu/intel_pstate/no_turbo"
			if [[ $(cat /sys/devices/system/cpu/intel_pstate/no_turbo) == 0 ]]; then
				echo -e "Succesfully restored no_turbo state: $NO_TURBO"
			else
				echo -e "Failed to restore no_turbo state: $NO_TURBO"
			fi
		fi
	else
		orig_array=($original_state)
		i=0
		for core in $cores; do
			orig_state=${orig_array[$i]}
			if [[ $orig_state == "0" ]]; then
				sudo wrmsr -p${core} 0x1a0 0x850089
			fi
			((i++))
		done

		echo "Restored no_turbo state: $(get_turbo_state_string)"
	fi
}

function check_no_turbo {
	if [ -e $NO_TURBO_FILE ]; then

		NO_TURBO=$(cat "$NO_TURBO_FILE")
		original_no_turbo=$NO_TURBO

		if [[ $NO_TURBO != 1 ]]; then
			sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
			if [[ $(cat /sys/devices/system/cpu/intel_pstate/no_turbo) == 1 ]]; then
				echo "Succesfully disabled turbo boost using intel_pstate/no_turbo"
			else
				echo "Failed to disable turbo boost using intel_pstate/no_turbo"
			fi
		else
			echo "intel_pstate/no_turbo reports that turbo is already disabled"
		fi

	else
		echo "Does not support intel_pstate"
		check_msrtools_installed
		cores=$(cat /proc/cpuinfo | grep processor | awk '{print $3}')
		echo "setting turbo state for cores $(echo $cores | tr '\n' ' ')"

		echo "Original no_turbo state: $original_state"

		for core in $cores; do
			sudo wrmsr -p${core} 0x1a0 0x4000850089
		done

		echo "Modified no_turbo state: $(get_turbo_state_string)"
	fi
}

echo "Driver: $SCALING_DRIVER, governor: $SCALING_GOVERNOR" 
echo -e "Vendor ID: $VENDOR_ID\nModel name: $MODEL_NAME"

############### Adjust the scaling governor to 'performance' to avoid sub-nominal clocking ##########

if [[ "$SCALING_GOVERNOR" != "performance" ]]; then
	original_governor=$SCALING_GOVERNOR
	echo -n "Changing scaling_governor to performance: "
	if ! sudo -n true 2>/dev/null; then echo ""; fi # write a newline if we are about to prompt for sudo
	sudo sh -c "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
	if [[ $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor) == "performance" ]]; then
		echo "SUCCESS";
	else
		echo "FAILED";
	fi
fi

lsmod | egrep -q "^msr " || { echo "loading msr kernel module"; sudo modprobe msr; }

################# Disable turbo boost #######################
check_no_turbo

################ Load the libpfc kernel module if necessary ##################

selected_timer=$(./uarch-bench "$@" --internal-dump-timer | tail -1)
echo "Using timer: $selected_timer"
if [ "$selected_timer" == "libpfc" ]; then
	echo "Reloading pfc.ko kernel module"
	make insmod
fi

./uarch-bench "$@"

# restore no turbo mode
restore_no_turbo

# restore the cpu governor if we changed it
if [[ $original_governor ]]; then
	echo -n "Reverting cpufreq governor to $original_governor: "
	sudo sh -c "echo $original_governor > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
	if [[ $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor) == "$original_governor" ]]; then
		echo "SUCCESS"; 
	else
		echo "FAILED";
	fi
fi
