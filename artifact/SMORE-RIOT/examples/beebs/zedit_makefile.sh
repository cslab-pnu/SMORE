#!/bin/bash

# 현재 디렉토리와 모든 하위 디렉토리에서 Makefile 탐색
find . -type f -name "main.c" | while read file; do
  # 백업 없이 원본 파일을 직접 수정
  sed -i 's/REPEAT_FACTOR/1/g' "$file"
  echo "Updated: $file"
done

# 현재 디렉토리의 모든 하위 디렉토리를 순회
for dir in */ ; do
    # 디렉토리 이름에서 마지막 슬래시 제거
    dirname="${dir%/}"

    # main.c 파일이 있는지 확인
    if [ -f "$dir/main.c" ]; then
        echo "Processing $dir/main.c: replacing 'aha-compress' with '$dirname'"

        # 문자열 'abc'를 디렉토리 이름으로 치환 (백업 없이 바로 덮어씀)
        sed -i "s/aha-compress/$dirname/g" "$dir/main.c"
    else
        echo "Skipping $dir: main.c not found."
    fi
done

