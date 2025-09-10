#!/bin/sh

for bench in ./"$1"/multiple/*.c; do
    [ -e "$bench" ] || continue

    if ! grep -q "$2" "$bench"; then
        continue
    fi

    mv "$bench" .
done

make flash
sleep 0.1s

for bench in ./*.c; do
    if ! grep -q "$2" "$bench"; then
        continue
    fi

    mv "$bench" ./"$1"/end/
done
