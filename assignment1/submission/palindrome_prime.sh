#!/bin/bash
# Q4: Palindrome check for any value, prime check for numeric values

# is_palindrome VALUE: reverse the string with rev and compare it with the original
is_palindrome() {
    local value=$1
    local reversed
    reversed=$(echo "$value" | rev)
    if [ "$value" == "$reversed" ]; then
        echo "$value is a Palindrome"
    else
        echo "$value is not a Palindrome"
    fi
}

# is_prime N: trial division by every i with i*i <= n
is_prime() {
    local n=$((10#$1))
    if (( n < 2 )); then
        echo "$1 is not a Prime number"
        return
    fi
    for (( i = 2; i * i <= n; i++ )); do
        if (( n % i == 0 )); then
            echo "$1 is not a Prime number"
            return
        fi
    done
    echo "$1 is a Prime number"
}

# Main: read a value, reject empty input
read -p "Enter a string or number: " value
if [ -z "$value" ]; then
    echo "Error: no input given"
    exit 1
fi

is_palindrome "$value"

# Only purely numeric input goes to is_prime
if [[ $value =~ ^[0-9]+$ ]]; then
    is_prime "$value"
else
    echo "$value is not numeric, so the prime check is skipped"
fi
