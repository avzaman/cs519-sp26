#!/bin/bash
gcc ./multithread-pagefault-averagetime.c -o ./multithread-pagefault-averagetime

threadc_settings=("4" "8" "12" "16" "20" "24" "28" "32" "36" "40" "44" "48")
pagec_settings=("16" "32" "48" "64" "80" "96" "112" "128" "144" "160" "172" "188")

threadc_default=24
pagec_default=96
iterc=100000

echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid

#modify the thread count
if [[ -d ./change_threadc ]]; then
    rm -r "./change_threadc"
fi

if [[ -d ./change_pagec ]]; then
    rm -r "./change_pagec"
fi



mkdir change_threadc
cd change_threadc
for threadc in "${threadc_settings[@]}"; do
    perf record -a -g ../multithread-pagefault-averagetime $threadc $pagec_default $iterc > out_change_threadc_$threadc
    perf report -g "graph,0.5,caller" > perf_change_threadc_$threadc
    perf record -a -g ../multithread-pagefault-averagetime $threadc $pagec_default $iterc aaaa > out_extent_change_threadc_$threadc
    perf report -g "graph,0.5,caller" > perf_extent_change_threadc_$threadc
	rm -r -f perf.data perf.data.old
done
cd ..
mkdir change_pagec
cd change_pagec
#modify the page count
for pagec in "${pagec_settings[@]}"; do
    perf record -a -g ../multithread-pagefault-averagetime $threadc_default $pagec $iterc > out_change_pagec_$pagec
    perf report -g "graph,0.5,caller" > change_threadc_$pagec > perf_change_pagec_$pagec
    perf record -a -g ../multithread-pagefault-averagetime $threadc_default $pagec $iterc aaaa > out_extent_change_pagec_$pagec
    perf report -g "graph,0.5,caller" > extent_change_threadc_$pagec > perf_extent_change_pagec_$pagec
	rm -r -f perf.data perf.data.old
done
cd ..

#data exfiltration
#mv ./change_threadc ~/p2data/test_results
#mv ./change_pagec ~/p2data/test_results
#cd ~/p2data
#./xfer.sh
