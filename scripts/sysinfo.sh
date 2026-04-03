#!/bin/sh

echo "Model: PDP-11/70 class system under SIMH emulation"
echo "Style: DEC 16-bit minicomputer, high end of the Unibus line"
echo "Era: introduced in 1975 for serious time-sharing and lab work"
echo "Hardware notes: 22-bit addressing, split I/D support, cache,"
echo "and the scale of a machine built for shared institutional use."
echo

phys_line=`sysctl hw.physmem 2>/dev/null`
physmem=`echo "$phys_line" | sed 's/[^0-9][^0-9]*$//' | sed 's/.*[^0-9]//'`
if [ "x$physmem" != x ]; then
    physmb=`expr \( "$physmem" + 1048575 \) / 1048576 2>/dev/null`
    echo "Installed physical memory: ${physmb} MB ($physmem bytes)"
fi

user_line=`sysctl hw.usermem 2>/dev/null`
usermem=`echo "$user_line" | sed 's/[^0-9][^0-9]*$//' | sed 's/.*[^0-9]//'`
if [ "x$usermem" != x ]; then
    usermb=`expr \( "$usermem" + 1048575 \) / 1048576 2>/dev/null`
    echo "User-process memory:       ${usermb} MB ($usermem bytes)"
fi

boottime=`sysctl kern.boottime 2>/dev/null | sed 's/^[^=]*= *//'`
if [ "x$boottime" != x ]; then
    echo "Boot time:                 $boottime"
fi
echo

root_line=`df / 2>/dev/null | sed -n '2p'`
if [ "x$root_line" != x ]; then
    set x $root_line
    root_dev=$2
    root_total=$3
    root_used=$4
    root_free=$5
    root_total_mb=`expr \( "$root_total" + 1023 \) / 1024 2>/dev/null`
    root_used_mb=`expr \( "$root_used" + 1023 \) / 1024 2>/dev/null`
    root_free_mb=`expr \( "$root_free" + 1023 \) / 1024 2>/dev/null`
    echo "Root filesystem (/):"
    echo "  $root_dev, total ${root_total_mb} MB, used ${root_used_mb} MB, free ${root_free_mb} MB"
fi

usr_line=`df /usr 2>/dev/null | sed -n '2p'`
if [ "x$usr_line" != x ]; then
    set x $usr_line
    usr_dev=$2
    usr_total=$3
    usr_used=$4
    usr_free=$5
    usr_total_mb=`expr \( "$usr_total" + 1023 \) / 1024 2>/dev/null`
    usr_used_mb=`expr \( "$usr_used" + 1023 \) / 1024 2>/dev/null`
    usr_free_mb=`expr \( "$usr_free" + 1023 \) / 1024 2>/dev/null`
    echo "User filesystem (/usr):"
    echo "  $usr_dev, total ${usr_total_mb} MB, used ${usr_used_mb} MB, free ${usr_free_mb} MB"
fi

bin_line=`du -sk /bin 2>/dev/null | sed -n '1p'`
etc_line=`du -sk /etc 2>/dev/null | sed -n '1p'`
lib_line=`du -sk /lib 2>/dev/null | sed -n '1p'`
if [ "x$bin_line" != x ] && [ "x$etc_line" != x ] && [ "x$lib_line" != x ]; then
    set x $bin_line
    bin_k=$2
    set x $etc_line
    etc_k=$2
    set x $lib_line
    lib_k=$2
    core_k=`expr "$bin_k" + "$etc_k" + "$lib_k" 2>/dev/null`
    core_mb=`expr \( "$core_k" + 1023 \) / 1024 2>/dev/null`
    echo "Core system files (/bin + /etc + /lib): ${core_mb} MB"
fi

kernel_line=`ls -l /unix 2>/dev/null | sed -n '1p'`
if [ "x$kernel_line" != x ]; then
    set x $kernel_line
    kernel_bytes=$5
    kernel_k=`expr \( "$kernel_bytes" + 1023 \) / 1024 2>/dev/null`
    echo "Kernel image (/unix): ${kernel_k} KB ($kernel_bytes bytes)"
fi
echo

echo "Storage layout: root and /usr live on separate BSD partitions,"
echo "a practical way to divide system software from user space on"
echo "a modest disk pack."
echo "The /usr filesystem holds most user commands, games, manuals,"
echo "libraries, and local additions, so it is the larger side of the"
echo "installation even when the core operating system is compact."
echo "SIMH note: Bob Supnik's emulator keeps systems like this alive"
echo "for study and daily use on modern hardware."
echo "PiDP-11 note: Oscar Vermeulen's front-panel recreation gives the"
echo "system a physical presence that makes the machine easier to read."
echo

echo "Network:"

hostname_value=`hostname 2>/dev/null | sed -n '1p'`
if [ "x$hostname_value" != x ]; then
    echo "  Hostname: $hostname_value"
fi

who_count=`who 2>/dev/null | wc -l | sed 's/ //g'`
if [ "x$who_count" != x ]; then
    echo "  Logged-in users: $who_count"
fi

echo "  Interfaces:"
netstat -i 2>/dev/null | sed -n '1,5p' | sed 's/^/    /'

route_line=`netstat -rn 2>/dev/null | sed -n '/^default /p;/^0.0.0.0 /p' | sed -n '1p'`
if [ "x$route_line" != x ]; then
    set x $route_line
    echo "  Default route: $3"
fi
