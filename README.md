# linuxのeventfdの性能測定

## 概要
2つのプロセス（親・子）を生成し、eventfd/epollを用いた通知・応答の往復遅延を測定します。

## 使い方

### ビルド
```sh
make
```

### 実行例
```sh
sudo ./eventfd_benchmark [オプション]
```

#### 主なオプション
- `--realtime <優先度>` : 親子プロセスをSCHED_FIFOリアルタイムクラス＋指定優先度で実行
- `--cpu <親コア番号> <子コア番号>` : 親・子プロセスを指定したCPUコアに割り当て
- `--mlock` : mlockallでメモリをロック

#### 使用例
```sh
sudo ./eventfd_benchmark --realtime 50 --cpu 2 3 --mlock
```
（親をCPU2、子をCPU3、リアルタイム優先度50、メモリロック有効で実行）

## 測定結果
平均・中央値・最小・最大応答時間（us単位）が表示されます。

## タスク遷移の調査
perf等でスケジューリングイベントを記録し、最大遅延時の状況を調査できます。
（例: trace_bench.sh 参照）



