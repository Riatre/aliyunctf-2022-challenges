#!/bin/bash -e

set -o pipefail

FLAG_PREFIX="aliyunctf{"
CONTAINER_GUY="${CONTAINER_GUY:-docker}"

should_not_crash_in_common_distro() {
    echo "The binary should run and not crash in common distro:"
    IMAGES_TO_TEST=(
        fedora:35
        fedora:36
        fedora:37
        fedora:rawhide
        almalinux:9
        debian:testing
        debian:unstable
        ubuntu:22.04
        ubuntu:22.10
        archlinux:latest
    )
    for image in "${IMAGES_TO_TEST[@]}"; do
        echo -n "Testing $image..."
        "$CONTAINER_GUY" run --rm -i -v "$PWD:/work" "$image" sh -c 'cd /work; echo 1 | /work/lyra' >/dev/null
        echo "OK"
    done
    echo
}

unable_to_run_with_older_glibc() {
    echo "The binary should not run with glibc < 2.34 (we actually require >= 2.27 but __libc_start_main@GLIBC_2.34 is a nice natural constaint)."
    ("$CONTAINER_GUY" run --rm -i -v "$PWD:/work" "centos:7" sh -c 'cd /work; echo 1 | /work/lyra' 2>&1 || true) | grep GLIBC_
    echo
}

should_be_solvable() {
    echo "The challenge should be solvable:"
    echo "cat flag.txt" | python3 solve.py | grep "$FLAG_PREFIX"
    echo
}

should_not_be_solved_without_correct_delay() {
    echo -n "The backdoor should not be triggered without proper delay... "
    # Should have used expect(1), but it gets job done /shrug
    if echo "cat flag.txt" | python3 solve.py DELAY=0 2>/dev/null | grep "$FLAG_PREFIX"; then
        echo "FAIL (DELAY=0)"
        exit 1
    fi
    if echo "cat flag.txt" | python3 solve.py DELAY=4 2>/dev/null | grep "$FLAG_PREFIX"; then
        echo "FAIL (DELAY=4)"
        exit 1
    fi
    echo "OK"
}

should_not_be_solved_with_wrong_key() {
    echo -n "The backdoor should not run without correct trigger... "
    if echo "cat flag.txt" | python3 solve.py WRONG_KEY 2>/dev/null | grep "$FLAG_PREFIX"; then
        echo "FAIL"
        exit 1
    fi
    echo "OK"
}

should_not_trigger_if_debugger_found() {
    echo -n "The backdoor should not be installed if debugger found... "
    export COLUMNS=233
    export LINES=80
    if echo "cat flag.txt" | python3 solve.py 2>/dev/null | grep "$FLAG_PREFIX"; then
        echo "FAIL (COLUMNS & LINES)"
        exit 1
    fi
    unset COLUMNS
    unset LINES

    export LD_LIBRARY_PATH=haha
    if echo "cat flag.txt" | python3 solve.py 2>/dev/null | grep "$FLAG_PREFIX"; then
        echo "FAIL (LD_*)"
        exit 1
    fi
    echo "OK"
}

should_not_trigger_if_argv_unconvincing() {
    echo -n "The backdoor should not be installed if argv[0][0] != '/'... "
    if echo "cat flag.txt" | python3 solve.py EXE=./lyra 2>/dev/null | grep "$FLAG_PREFIX"; then
        echo "FAIL"
        exit 1
    fi
    echo "OK"
}

should_not_crash_in_common_distro
unable_to_run_with_older_glibc
should_be_solvable
should_not_be_solved_without_correct_delay
should_not_be_solved_with_wrong_key
should_not_trigger_if_argv_unconvincing
