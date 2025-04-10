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
SETUP=("-DSETUP" "-DKEY_CONFIG_SETUP" "-DADD_CX_SETUP" "-DINIT_SETUP")
EXPERIMENT=("-DCREATE" "-DKEY_CONFIG" "-DADD_CX" "-DINIT")

# Output CSV file to store execution times
output_file="execution_times.csv"
temp_file="temp.txt"

echo "INSTR, PROOF_TYPE, TIME, MEMORY" > $output_file

#
for i in "${!EXPERIMENT[@]}"; do
    for j in "${!PARAMETERS[@]}"; do
        #process_command="cbmc ${PARAMETERS[$j]} ${SETUP[$i]}} ${EXPERIMENT[$i]} driver/driver.c --function driver_main $FILES -I "$PWD" --trace" 
        #time_command="/usr/bin/time -h -l $process_command"
        #$time_command > $temp_file
        # Extract user time (e.g., "0.00s user")
        user_time=$(grep -oP '\d+\.\d+s(?=\s+user)' "$temp_file")

        # Extract maximum resident set size
        max_rss=$(grep 'maximum resident set size' "$temp_file" | awk '{print $1}')

        echo "${EXPERIMENT[$i]}, ${PARAMETERS[$j]}, $user_time, $max_rss" >> $output_file
    done
done

rm -f $temp_file