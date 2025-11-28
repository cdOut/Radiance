#ifndef INTERVAL_H
#define INTERVAL_H

#include <limits>

const float infinity = std::numeric_limits<float>::infinity();

class Interval {
    public:
        Interval() : _min(+infinity), _max(-infinity) {}

        Interval(float min, float max) : _min(min), _max(max) {}

        float size() const {
            return _max - _min;
        }

        bool contains(float value) const {
            return _min <= value && value <= _max;
        }

        bool surrounds(float value) const {
            return _min < value && value < _max;
        }

        float clamp(float value) const {
            if (value < _min) return _min;
            if (value > _max) return _max;
            return value;
        }

        float& min() { return _min; }
        float& max() { return _max; }

        static const Interval empty, universe;
    private:
        float _min, _max;
};

const Interval Interval::empty = Interval(+infinity, -infinity);
const Interval Interval::universe = Interval(-infinity, +infinity);

#endif