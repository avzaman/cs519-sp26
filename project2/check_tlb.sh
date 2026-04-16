#!/bin/bash
echo "---------------------------"
echo "This script captures the extent count and physical page count of the last extent-tree-report system call."
echo "Alternatively, provide a command as a parameter and it will use that as a basis."
echo "---------------------------"
eval "$*"
pgcount=`sudo dmesg | egrep -o "total pages = [0-9]{1,}" | tail -1 | egrep -o "[0-9]{1,}"`
extent_c=`sudo dmesg | egrep -o "total extents = [0-9]{1,}" | tail -1 | egrep -o "[0-9]{1,}"`
echo "Process used ${extent_c} extents at time of last report"
echo "Process had  ${pgcount} physical pages at time of last report"
tlb_save=$((pgcount-extent_c))
echo "If, right now, each extent was put into the TLB instead of using each 1-1 VPN->PFN mapping,"
echo "there would be ${tlb_save} fewer entries needed in the TLB."
