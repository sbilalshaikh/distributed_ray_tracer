#ifndef INTERVAL_H
#define INTERVAL_H

#include <limits>

class interval {
  public:
    double min, max;

    interval() : min(+std::numeric_limits<double>::infinity()), max(-std::numeric_limits<double>::infinity()) {} // Default interval is empty

    interval(double min, double max) : min(min), max(max) {}

    bool contains(double x) const {
        return min <= x && x <= max;
    }

    bool surrounds(double x) const {
        return min < x && x < max;
    }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const interval empty, universe;
};

const inline interval interval::empty    = interval(+std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());
const inline interval interval::universe = interval(-std::numeric_limits<double>::infinity(), +std::numeric_limits<double>::infinity());

#endif