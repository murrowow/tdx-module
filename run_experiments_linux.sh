#!/bin/bash

# files
FILES="src/common/memory_handlers/keyhole_manager.c \
       src/common/memory_handlers/pamt_manager.c \
       src/common/helpers/helpers.c \
       include/auto_gen/op_state_lookup.c \
       driver/driver.c \
       driver/flows/flows.c \
       src/vmm_dispatcher/api_calls/tdh_mng_create.c \
       src/vmm_dispatcher/api_calls/tdh_mng_key_config.c \
       src/vmm_dispatcher/api_calls/tdh_mng_add_cx.c \
       src/vmm_dispatcher/api_calls/tdh_mng_init.c"

PARAMETERS=("-DMODULAR_PROOF" "-DFLOW_PROOF")
SETUP=("-DSETUP" "-DSETUP" "-DKEY_CONFIG_SETUP" "-DADD_CX_SETUP" "-DINIT_SETUP")
EXPERIMENT=("-DWHOLE_FLOW" "-DCREATE" "-DKEY_CONFIG" "-DADD_CX" "-DINIT")

# Output CSV file to store execution times
output_file="execution_times.csv"
temp_file="temp.txt"

echo "INSTR,PROOF_TYPE,TIME,MEMORY" > "$output_file"

for i in "${!EXPERIMENT[@]}"; do
    for j in "${!PARAMETERS[@]}"; do

        if [ "${EXPERIMENT[$i]}" = "-DWHOLE_FLOW" ] && [ "${PARAMETERS[$j]}" = "-DMODULAR_PROOF" ]; then
            continue
        fi

        process_command="cbmc ${PARAMETERS[$j]} ${SETUP[$i]} ${EXPERIMENT[$i]} driver/driver.c $FILES -I \"$PWD\" --function driver_main --trace"
        time_command="/usr/bin/time -v $process_command"
        eval "$time_command" 2> "$temp_file"
        # Extract user time (e.g., "User time (seconds): 0.00")
        user_time=$(awk -F': ' '/User time/ {print $2}' "$temp_file")

        # Extract max resident set size (in kilobytes)
        max_rss=$(awk -F': ' '/Maximum resident set size/ {print $2}' "$temp_file")

        echo "${EXPERIMENT[$i]},${PARAMETERS[$j]},$user_time,$max_rss" >> "$output_file"

    done
done

rm -f "$temp_file"
