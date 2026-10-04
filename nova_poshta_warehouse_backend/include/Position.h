#pragma once

struct Position {
    double x{0.0};
    double y{0.0};

    Position() = default;
    Position(double xValue, double yValue) : x(xValue), y(yValue) {}
};
