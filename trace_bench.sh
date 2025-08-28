#!/bin/bash
# eventfdベンチマーク最大遅延時のタスク遷移を調べるスクリプト
# 必要: perf, sudo権限

BENCH=./eventfd_benchmark
BENCH_OPTS="--realtime 50 --cpu 2 3 --mlock"
PERF_DATA=perf.data
PERF_SCRIPT=perf_script.txt

# 1. perfでスケジューリングイベントを記録
sudo perf sched record -e sched:sched_switch -aR &
PERF_PID=$!

# 2. ベンチマーク実行（最大応答時刻も出力するようにしておくと良い）
echo "Running benchmark: $BENCH $BENCH_OPTS"
sudo $BENCH $BENCH_OPTS

# 3. perf記録停止
sudo kill $PERF_PID
wait $PERF_PID 2>/dev/null

# 4. perfスクリプト出力
sudo perf script > $PERF_SCRIPT

echo "perf script output saved to $PERF_SCRIPT"
echo "最大応答時刻前後のスケジューリングイベントをgrep等で調査してください。"
