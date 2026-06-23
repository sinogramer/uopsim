# uopsim

x86 マイクロアーキテクチャの μop スケジューリングを模擬し、命令列の理論サイクル数を算出するシミュレータ。

複数のプロセッサ（Zen4, Zen5 など）に対して、同じアルゴリズムを流して理論性能を比較できます。

## 特徴

- **μop 単位のスケジューリング**：マクロ命令を μop に分解し、ポート割当・実行レイテンシ・リオーダバッファ・スケジューラキューの制約をモデル化
- **CPU パラメータ化**：ROB サイズ、ディスパッチ幅、ポート数などを `CpuModel` 構造体で記述
- **ISA 抽象化**：同じ命令でも CPU ごとに異なる μop 構成・ポート割当を `Isa` インタフェースで切り替え可能
- **アルゴリズムの CPU 非依存記述**：`polyvalx8(Scheduler&, Isa&, ...)` のように記述しておけば、CPU を切り替えるだけで再測定可能

## ディレクトリ構造

```
uopsim/
├── include/
│   ├── core/          # コアライブラリ（スケジューラ・命令クラス等）
│   │   ├── cpu_model.hpp
│   │   ├── uops.hpp
│   │   ├── argument.hpp
│   │   ├── scheduler.hpp
│   │   ├── instruction.hpp
│   │   ├── isa.hpp
│   │   └── header.hpp
│   ├── cpus/          # CPU パラメータ定義
│   │   ├── zen4.hpp
│   │   └── zen5.hpp
│   ├── isa/           # CPU 別の命令テーブル
│   │   ├── zen4_isa.hpp
│   │   └── zen5_isa.hpp
│   └── algo/          # アルゴリズム実装（ヘッダオンリー）
│       ├── polyval.hpp
│       └── xctr.hpp
├── apps/              # 実行ファイル
│   ├── polyval_bench.cpp
│   ├── xctr_bench.cpp
│   └── compare_cpus.cpp
├── CMakeLists.txt
└── README.md
```

## 必要なもの

- C++17 対応のコンパイラ（GCC 7+, Clang 5+ など）
- CMake 3.15 以上

## ビルド方法

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

最適化を有効にしたい場合は：

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## 使い方

### 単一 CPU でアルゴリズムを測定

```bash
./build/polyval_bench
```

出力例：

```
CPU: Zen4, total cycles = 12345, cycles/byte = 0.118
```

### CPU 間の性能比較

```bash
./build/compare_cpus
```

出力例：

```
=== polyvalx8 ===
Zen4   :    12345 cycles, 0.1180 c/B
Zen5   :    10123 cycles, 0.0965 c/B
```

## 基本的な使い方（コード例）

```cpp
#include "algo/polyval.hpp"
#include "cpus/zen4.hpp"
#include "isa/zen4_isa.hpp"

int main() {
    Scheduler sched(ZEN4);
    int num_blocks = 65536;

    algo::polyval::polyvalx8(sched, isa::zen4::instance(), num_blocks);

    std::printf("total cycles = %d\n", sched.total_cycles());
}
```

### CPU を切り替える

```cpp
Scheduler sched_zen5(ZEN5);
algo::polyval::polyvalx8(sched_zen5, isa::zen5::instance(), num_blocks);
```

## 新しい CPU を追加する

1. `include/cpus/xxx.hpp` を作成し `CpuModel` を定義

```cpp
inline const CpuModel XXX = {
    "XXX", /*num_ports=*/4, /*dispatch_width=*/6,
    /*rob_size=*/320, /*schedq_size=*/64,
};
```

2. `include/isa/xxx_isa.hpp` を作成し `Isa` を継承して命令テーブルを実装

```cpp
namespace isa::xxx {
class XxxIsa : public Isa {
public:
    instruction& pxor() override { return pxor_; }
    // ...
private:
    instruction pxor_{{{ uops(1, 0b1111, "  pxor   ") }}};
    // ...
};
inline XxxIsa& instance() { static XxxIsa inst; return inst; }
}
```

3. 利用側で `Scheduler sched(XXX); algo::polyval::polyvalx8(sched, isa::xxx::instance(), n);` のように呼ぶ

## 新しいアルゴリズムを追加する

1. `include/algo/yyy.hpp` を作成し、`Scheduler&, Isa&` を引数に取るアルゴリズム関数を `inline` で実装
2. `apps/yyy_bench.cpp` を作成して `main()` を書く
3. `CMakeLists.txt` に実行ファイルを追加

```cmake
add_executable(yyy_bench apps/yyy_bench.cpp)
target_link_libraries(yyy_bench PRIVATE uopsim_core)
```

## モデルの概要

各マクロ命令は、複数のステップ（μop グループ）の直列実行として記述されます：

- **グループ内の μop**：同一サイクル内で並列にポートに割当
- **グループ間**：前のグループが完了してから次が開始

スケジューリング時には以下の制約を考慮：

| 制約 | 影響 |
|---|---|
| ディスパッチ幅 | 1サイクルあたりのマクロ命令数の上限 |
| ROB サイズ | 未完了命令の総数の上限 |
| スケジューラキュー（SCHEDQ） | 待機中 μop 数の上限 |
| ポート競合 | 同一サイクル・同一ポートに割当不可 |
| 実行リソース（例: MROM, 共有 FPU 等） | 専用リソースの reciprocal throughput |

## 既知の制限

- フロントエンド（fetch, decode）のモデル化は簡略化
- 分岐予測・キャッシュミスは扱わない
- メモリレイテンシは固定値として `uops` の `cycles` で表現
- 純粋に **理論最大値** を求めるツールであり、実機計測の代替ではない

## 今後の拡張候補

- xctr の実装
- AES 系命令のサポート
- ガントチャート的な可視化出力
- 命令テーブルを YAML/JSON から読み込み
- 単体テスト（GoogleTest/Catch2）の追加


## License

研究用途で自由に利用可。学術発表で参照する場合は連絡してください。


# 参考

- [uops.info](https://uops.info/) - 各 CPU の μop 構成情報
- [Agner Fog's optimization manuals](https://www.agner.org/optimize/) - マイクロアーキテクチャの詳細
- [AMD Software Optimization Guide for AMD Zen 4](https://www.amd.com/...) - Zen4 公式資料
