/*データ（引数） 依存関係を管理する*/
#pragma once

class argument {
public:
    void set_cycle(int cycle) { prev_cycle = cycle; }
    int  get_prev_cycle() const { return prev_cycle; }
private:
    int prev_cycle = 0;
};
