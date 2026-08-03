#pragma once

namespace game {

class Credits {
public:
    void Add(int n = 1) { count_ += n; }
    void Set(int n) { count_ = n < 0 ? 0 : n; }
    bool Has() const { return count_ > 0; }
    bool Consume() {
        if (count_ <= 0) {
            return false;
        }
        --count_;
        return true;
    }
    int count() const { return count_; }

private:
    int count_ = 0;
};

}  // namespace game
