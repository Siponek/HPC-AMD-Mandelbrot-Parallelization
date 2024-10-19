#!/bin/bash

# USABLE ONLY ON SOME NODE< NOT FRONTEND
HOST_PREFIX="hpcocapie"
MACHINEFILE="machinefile.txt"

# Fetch the list of hosts with the specified prefix
AVAILABLE_HOSTS=$(bhosts | grep "^${HOST_PREFIX}" | awk '$2 == "ok" {print $1}')

# Check if any hosts are available
if [ -z "$AVAILABLE_HOSTS" ]; then
    echo "No available hosts found with prefix ${HOST_PREFIX}."
    exit 1
fi

SHORT_HOSTS=$(echo "$AVAILABLE_HOSTS" | sed 's/\..*//')

# Verify that short hostnames are unique (optional but recommended)
UNIQUE_SHORT_HOSTS=$(echo "$SHORT_HOSTS" | sort | uniq)

echo "Creating ${MACHINEFILE} with available hosts:"
echo "$UNIQUE_SHORT_HOSTS" > "$MACHINEFILE"

cat "$MACHINEFILE"
