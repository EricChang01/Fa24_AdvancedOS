for i in {1..5}
do
    taskset -c 0 sudo ./perf_event 1 1 1 1 1
done
