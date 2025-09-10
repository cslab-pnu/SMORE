#!/bin/sh

mkdir -p ./"$1"/multiple
mkdir -p ./"$1"/end

for bench in ./"$1"/*.c; do
    [ -e "$bench" ] || continue
    bench_name=$(basename "$bench")

    if echo "$bench_name" | grep -qE 'a\.c|b\.c|c\.c|d\.c|e\.c'; then
        mv "$bench" ./"$1"/multiple/
    else
        mv "$bench" .
        make flash
        sleep 0.1s
        mv "$bench_name" ./"$1"/end/
    fi
done

