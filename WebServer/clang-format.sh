find . -name "*.h" -o -name "*.cpp" | grep -v build |grep -v thirt_part| xargs clang-format -style=google -i