ulimit -s  256
ulimit -i  120000
echo 120000 > /proc/sys/kernel/threads-max
echo 600000 > /proc/sys/vm/max_map_count
echo 200000 > /proc/sys/kernel/pid_max 
