#pragma once

#include <string>

class Product {
private:
    int id_{};
    std::string name_;
    std::string sku_;
    double weight_{};
    double volume_{};

public:
    Product() = default;
    Product(int id,
            const std::string& name,
            const std::string& sku,
            double weight,
            double volume);

    int getId() const;
    const std::string& getName() const;
    const std::string& getSku() const;
    double getWeight() const;
    double getVolume() const;

    void setName(const std::string& name);
    void setSku(const std::string& sku);
    void setWeight(double weight);
    void setVolume(double volume);
};
