#!/bin/bash
# Copyright 2017-2026 Matt "MateoConLechuga" Waltz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

for d in ./*/
do
    ( cd "$d" && echo "[test] `pwd`" ; ../../bin/convimg -i convimg.yaml ) || { exit 1; }
done

# thread pool stress: multithreaded output must match the single-threaded
# reference; detection is probabilistic so run several iterations
(
    cd threads &&
    echo "[test] `pwd` (thread stress)" &&
    mkdir -p ref &&
    ../../bin/convimg -i convimg.yaml -t 1 > /dev/null &&
    for f in *.bin; do cp "$f" "ref/$f"; done &&
    for i in $(seq 1 20)
    do
        ../../bin/convimg -i convimg.yaml -t 8 > /dev/null || { echo "[test] thread stress crashed (iteration $i)"; exit 1; }
        for f in *.bin
        do
            cmp -s "$f" "ref/$f" || { echo "[test] thread stress output mismatch: $f (iteration $i)"; exit 1; }
        done
    done &&
    rm -rf ref
) || { exit 1; }
