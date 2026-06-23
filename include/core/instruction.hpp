#pragma once
#include "core/header.hpp"
#include "core/uops.hpp"
#include "core/argument.hpp"
#include "core/scheduler.hpp"

class instruction {
public:
    instruction(const std::vector<std::vector<uops>>& ops_groups,
                int exec_throughput_limit = 1,
                std::string exec_resource_name = "",
                int mrom_limit = 0)
        : micro_ops_groups(ops_groups),
          exec_throughput_limit(exec_throughput_limit),
          exec_resource_name(std::move(exec_resource_name)),
          mrom_limit(mrom_limit) {}

    // instruction(const std::vector<uops>& ops,
    //             int exec_throughput_limit = 1,
    //             std::string exec_resource_name = "",
    //             int mrom_limit = 0)
    //     : exec_throughput_limit(exec_throughput_limit),
    //       exec_resource_name(std::move(exec_resource_name)),
    //       mrom_limit(mrom_limit) {
    //     for (const auto& op : ops) micro_ops_groups.push_back({op});
    // }

    void exe(Scheduler& sched, argument& output, argument& input0) {
        std::vector<argument*> in = {&input0};
        exe_core(sched, output, in);
    }
    void exe(Scheduler& sched, argument& output, argument& input0, argument& input1) {
        std::vector<argument*> in = {&input0, &input1};
        exe_core(sched, output, in);
    }
    void exe(Scheduler& sched, argument& output,
             argument& input0, argument& input1, argument& input2) {
        std::vector<argument*> in = {&input0, &input1, &input2};
        exe_core(sched, output, in);
    }

    int get_cycles() const {
        int sum = 0;
        for (const auto& g : micro_ops_groups)
            for (const auto& op : g) sum += op.get_cycles();
        return sum;
    }

private:
    std::vector<std::vector<uops>> micro_ops_groups;
    int exec_throughput_limit;
    std::string exec_resource_name;
    int mrom_limit;

    void exe_core(Scheduler& sched, argument& output,
                  const std::vector<argument*>& in_args) {
        const auto& cpu = sched.cpu();

        int base_prev_cycle = 0;
        for (argument* a : in_args)
            base_prev_cycle = std::max(base_prev_cycle, a->get_prev_cycle());

        // ディスパッチ幅チェック
        if (sched.dispatched_in_current_cycle() >= cpu.dispatch_width) {
            sched.advance_dispatch_cycle();
            sched.reset_dispatched_in_current_cycle();
        }

        // ROB制約
        if (sched.global_inst_count() >= cpu.rob_size) {
            int rob_free_cycle = sched.inst_completion_cycle(
                sched.global_inst_count() - cpu.rob_size);
            if (sched.current_dispatch_cycle() < rob_free_cycle) {
                sched.set_current_dispatch_cycle(rob_free_cycle);
                sched.reset_dispatched_in_current_cycle();
            }
        }

        // MROM制約
        if (mrom_limit > 0) {
            int ready = sched.resource_ready("MROM");
            if (sched.current_dispatch_cycle() < ready) {
                sched.set_current_dispatch_cycle(ready);
                sched.reset_dispatched_in_current_cycle();
            }
            sched.set_resource_ready("MROM",
                sched.current_dispatch_cycle() + mrom_limit);
        }

        int step_start_cycle = std::max(base_prev_cycle, sched.current_dispatch_cycle());

        if (!exec_resource_name.empty()) {
            int exec_cycle = sched.resource_ready(exec_resource_name);
            step_start_cycle = std::max(step_start_cycle, exec_cycle);
            sched.set_resource_ready(exec_resource_name,
                step_start_cycle + exec_throughput_limit);
        }

        int final_complete_cycle = 0;

        for (const auto& group : micro_ops_groups) {
            int group_max_complete = step_start_cycle;

            for (const auto& op : group) {
                // SCHEDQ満杯チェック
                while (sched.schedq_size_at(sched.current_dispatch_cycle())
                       >= cpu.schedq_size) {
                    sched.advance_dispatch_cycle();
                    sched.reset_dispatched_in_current_cycle();
                }

                int my_dispatch_cycle = sched.current_dispatch_cycle();
                int uop_start_cycle =
                    std::max(step_start_cycle, sched.current_dispatch_cycle());

                uint8_t allowed_ports = op.get_allowed_ports() & 0x0F;
                int min_cycle = -1;
                int avail_port = -1;

                if (allowed_ports == 0) {
                    min_cycle = uop_start_cycle;
                } else {
                    std::vector<int> port_order;
                    if (allowed_ports == 0b1111)      port_order = {3, 0, 1, 2};
                    else if (allowed_ports == 0b1110) port_order = {3, 2, 1};
                    else {
                        for (int p = 0; p < cpu.num_ports; ++p)
                            if ((allowed_ports >> p) & 1)
                                port_order.push_back(p);
                    }

                    for (int port : port_order) {
                        int cycle = uop_start_cycle;
                        while (sched.port_busy(cycle, port)) cycle++;
                        if (min_cycle == -1 || cycle < min_cycle) {
                            min_cycle = cycle;
                            avail_port = port;
                        }
                    }

                    sched.assign_port(min_cycle, avail_port, op);
                    for (int c = my_dispatch_cycle; c < min_cycle; ++c)
                        sched.inc_schedq(c);
                }

                int complete_cycle = min_cycle + op.get_cycles();
                group_max_complete = std::max(group_max_complete, complete_cycle);
            }

            step_start_cycle = group_max_complete;
            final_complete_cycle = group_max_complete;
        }

        sched.push_inst_completion(final_complete_cycle);
        output.set_cycle(final_complete_cycle);
        sched.inc_global_inst_count();
        sched.inc_dispatched_in_current_cycle();
        sched.update_total_cycles(final_complete_cycle);
    }
};