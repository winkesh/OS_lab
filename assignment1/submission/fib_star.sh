#!/bin/bash
# Q3: First N Fibonacci terms and an N-row right-angled star triangle

# Read N and make sure it is a positive integer
read -p "Enter N: " n
if ! [[ $n =~ ^[0-9]+$ ]] || [ "$n" -eq 0 ]; then
    echo "Error: N must be a positive integer"
    exit 1
fi

# Fibonacci: start with 0 and 1, each new term is the sum of the previous two
a=0
b=1
echo "First $n Fibonacci terms:"
for (( i = 1; i <= n; i++ )); do
    if (( i < n )); then
        printf "%d, " "$a"
    else
        printf "%d\n" "$a"
    fi
    next=$(( a + b ))
    a=$b
    b=$next
done

# Star pattern: outer loop picks the row, inner loop prints that many stars
echo "Star pattern with $n rows:"
for (( row = 1; row <= n; row++ )); do
    for (( star = 1; star <= row; star++ )); do
        printf "*"
    done
    printf "\n"
done
