CPU=`cat /proc/cpuinfo | grep "model name" | head -n  1 | cut -d':' -f 2`

echo "CPU is: $CPU"
echo "Executing benchmark..."

echo "CPU is: $CPU" > results.txt
./benchmark | tee -a results.txt
