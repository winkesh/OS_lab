#!/bin/bash
# Q1: Simple Interest calculator
# Usage: ./si.sh <principal> <rate> <time_in_years>

# Exactly 3 arguments are required
if [ $# -ne 3 ]; then
    echo "Usage: $0 <principal> <rate> <time_in_years>"
    echo "Example: $0 10000 5 2"
    exit 1
fi

p=$1
r=$2
t=$3

# Each argument must be a non-negative number (integer or decimal)
for v in "$p" "$r" "$t"; do
    if ! [[ $v =~ ^[0-9]+([.][0-9]+)?$ ]]; then
        echo "Error: '$v' is not a valid non-negative number"
        exit 1
    fi
done

# SI = (P x R x T) / 100, computed with bc so decimals work
si=$(echo "scale=2; ($p * $r * $t) / 100" | bc)
total=$(echo "scale=2; $p + $si" | bc)

# Drop a trailing .00 so whole results print as integers
si=${si%.00}
total=${total%.00}

echo "Principal          = $p"
echo "Rate of Interest   = $r %"
echo "Time               = $t years"
echo "Simple Interest    = $si"
echo "Total Amount       = $total"
