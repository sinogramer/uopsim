#pragma once
#include "core/header.hpp"
#include "core/cpu_model.hpp"
#include "core/uops.hpp"


class Scheduler {
public:
    explicit Scheduler(const CpuModel& cpu)
        : cpu_(cpu),
          schedq_count_(1000, 0),
          port_schedule_(1000, std::vector<std::optional<uops>>(cpu.num_ports))
    {}

    const CpuModel& cpu() const { return cpu_; }

    int total_cycles() const { return total_cycles_; }

    // instruction が使うインタフェース群
    int  current_dispatch_cycle() const { return current_dispatch_cycle_; }
    void set_current_dispatch_cycle(int c) { current_dispatch_cycle_ = c; }
    void advance_dispatch_cycle()          { current_dispatch_cycle_++; }

    int  dispatched_in_current_cycle() const { return dispatched_in_current_cycle_; }
    void reset_dispatched_in_current_cycle() { dispatched_in_current_cycle_ = 0; }
    void inc_dispatched_in_current_cycle()   { dispatched_in_current_cycle_++; }

    int  global_inst_count() const { return global_inst_count_; }
    void inc_global_inst_count()   { global_inst_count_++; }

    int  inst_completion_cycle(int idx) const { return inst_completion_cycles_[idx]; }
    void push_inst_completion(int c)          { inst_completion_cycles_.push_back(c); }

    int  resource_ready(const std::string& k) {
        auto it = resource_ready_cycle_.find(k);
        return it == resource_ready_cycle_.end() ? 0 : it->second;
    }
    void set_resource_ready(const std::string& k, int c) {
        resource_ready_cycle_[k] = c;
    }

    int  schedq_size_at(int c) {
        ensure_schedq_size(c);
        return schedq_count_[c];
    }
    void inc_schedq(int c) {
        ensure_schedq_size(c);
        schedq_count_[c]++;
    }

    bool port_busy(int cycle, int port) {
        ensure_port_size(cycle);
        return port_schedule_[cycle][port].has_value();
    }
    void assign_port(int cycle, int port, const uops& op) {
        ensure_port_size(cycle);
        port_schedule_[cycle][port] = op;
    }

    void update_total_cycles(int c) {
        if (c > total_cycles_) total_cycles_ = c;
    }

    
// 縦方向（サイクルを縦軸、ポートを横軸）に出力する
    void print_schedule(int start_cycle = 0,
                        int end_cycle = -1,
                        std::FILE* out = stdout) const {
        if (end_cycle < 0) end_cycle = total_cycles_;
        if (end_cycle > (int)port_schedule_.size())
            end_cycle = (int)port_schedule_.size();

        constexpr int CELL_WIDTH = 10;       // uops名(9文字) + 区切り1
        constexpr const char* EMPTY = "    .    ";

        // --- ヘッダ ---
        std::fprintf(out, "Cycle |");
        for (int p = 0; p < cpu_.num_ports; ++p) {
            std::fprintf(out, " %*s%-*d",
                        (CELL_WIDTH - 5) / 2, "Port",
                        CELL_WIDTH - (CELL_WIDTH - 5) / 2 - 4, p);
        }
        std::fprintf(out, " | SchedQ\n");

        // --- 区切り線 ---
        std::fprintf(out, "------+");
        for (int p = 0; p < cpu_.num_ports; ++p) {
            for (int k = 0; k < CELL_WIDTH + 1; ++k) std::fputc('-', out);
        }
        std::fprintf(out, "-+-------\n");

        // --- 各サイクル行 ---
        for (int c = start_cycle; c < end_cycle; ++c) {
            std::fprintf(out, "%5d |", c);
            for (int p = 0; p < cpu_.num_ports; ++p) {
                if (port_schedule_[c][p].has_value()) {
                    const auto& op = port_schedule_[c][p].value();
                    std::fprintf(out, " %s", op.get_name().c_str());
                } else {
                    std::fprintf(out, " %s", EMPTY);
                }
            }
            int q = (c < (int)schedq_count_.size()) ? schedq_count_[c] : 0;
            std::fprintf(out, " | %5d\n", q);
        }
        
        // // 各ポートの使用率
        // std::fprintf(out, "\n--- Port Utilization ---\n");
        // for (int p = 0; p < cpu_.num_ports; ++p) {
        //     int used = 0;
        //     for (int c = start_cycle; c < end_cycle; ++c) {
        //         if (port_schedule_[c][p].has_value()) used++;
        //     }
        //     double rate = 100.0 * used / (end_cycle - start_cycle);
        //     std::fprintf(out, "Port%d: %5.1f%% (%d / %d)\n",
        //                 p, rate, used, end_cycle - start_cycle);
        // }

    }

    // ポート使用率と SchedQ 占有率を出力
    // 範囲を指定しなければ 0 ~ total_cycles 全体を集計
    void print_utilization(int start_cycle = 0,
                        int end_cycle = -1,
                        std::FILE* out = stdout) const {
        if (end_cycle < 0) end_cycle = total_cycles_;
        if (end_cycle > (int)port_schedule_.size())
            end_cycle = (int)port_schedule_.size();

        int span = end_cycle - start_cycle;
        if (span <= 0) {
            std::fprintf(out, "(empty range)\n");
            return;
        }

        std::fprintf(out, "=== Utilization (cycles %d..%d, span=%d) ===\n",
                    start_cycle, end_cycle, span);

        // --- 各ポートの使用サイクル数 ---
        int total_used = 0;
        for (int p = 0; p < cpu_.num_ports; ++p) {
            int used = 0;
            for (int c = start_cycle; c < end_cycle; ++c) {
                if (port_schedule_[c][p].has_value()) used++;
            }
            double rate = 100.0 * used / span;
            std::fprintf(out, "  Port%d : %6.2f%%  (%6d / %d)\n",
                        p, rate, used, span);
            total_used += used;
        }

        // --- 全ポート平均 ---
        double avg = 100.0 * total_used / (span * cpu_.num_ports);
        std::fprintf(out, "  -----\n");
        std::fprintf(out, "  Avg   : %6.2f%%  (%6d / %d)\n",
                    avg, total_used, span * cpu_.num_ports);

        // --- SchedQ 占有 ---
        int q_max = 0, q_sum = 0;
        int q_len = std::min(end_cycle, (int)schedq_count_.size());
        for (int c = start_cycle; c < q_len; ++c) {
            q_max = std::max(q_max, schedq_count_[c]);
            q_sum += schedq_count_[c];
        }
        double q_avg = (q_len > start_cycle)
                        ? (double)q_sum / (q_len - start_cycle)
                        : 0.0;
        std::fprintf(out, "  SchedQ: max=%d  avg=%.2f  (capacity=%d)\n",
                    q_max, q_avg, cpu_.schedq_size);
    }

private:
    void ensure_schedq_size(int c) {
        if (c >= (int)schedq_count_.size())
            schedq_count_.resize(schedq_count_.size() + 500, 0);
    }
    void ensure_port_size(int c) {
        if (c >= (int)port_schedule_.size())
            port_schedule_.resize(port_schedule_.size() + 500,
                                  std::vector<std::optional<uops>>(cpu_.num_ports));
    }

    const CpuModel& cpu_;

    int total_cycles_                = 0;
    int current_dispatch_cycle_      = 0;
    int dispatched_in_current_cycle_ = 0;
    int global_inst_count_           = 0;

    std::vector<int> inst_completion_cycles_;
    std::vector<int> schedq_count_;
    std::vector<std::vector<std::optional<uops>>> port_schedule_;
    std::unordered_map<std::string, int> resource_ready_cycle_;
};