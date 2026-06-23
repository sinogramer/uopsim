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