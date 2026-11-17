#!/bin/bash
# Q2: Leap year check + even/odd and positive/negative/zero classification

# Read the year and make sure it is a positive integer
read -p "Enter a year: " year
if ! [[ $year =~ ^[0-9]+$ ]] || [ "$year" -eq 0 ]; then
    echo "Error: '$year' is not a valid year"
    exit 1
fi

# Leap year: divisible by 4 and (not divisible by 100 or divisible by 400)
if (( year % 4 == 0 && (year % 100 != 0 || year % 400 == 0) )); then
    echo "$year is a Leap Year"
else
    echo "$year is not a Leap Year"
fi

# Read a number; an optional leading minus sign is allowed
read -p "Enter a number: " num
if ! [[ $num =~ ^-?[0-9]+$ ]]; then
    echo "Error: '$num' is not a valid integer"
    exit 1
fi

# Even or odd (bash % keeps the sign, so compare with 0)
if (( num % 2 == 0 )); then
    echo "$num is Even"
else
    echo "$num is Odd"
fi

# Positive, negative or zero
if (( num > 0 )); then
    echo "$num is Positive"
elif (( num < 0 )); then
    echo "$num is Negative"
else
    echo "$num is Zero"
fi
