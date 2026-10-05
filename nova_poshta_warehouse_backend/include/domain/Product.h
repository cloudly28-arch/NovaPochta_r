#pragma once

#include <string>

class Product {
private:
    int id_{};
    std::string name_;
    std::string category_;
    std::string sku_;
    double weight_{};
    double volume_{};

public:
    Product() = default;
    Product(int id,
            std::string name,
            std::string category = {},
            std::string sku = {},
            double weight = 0.0,
            double volume = 0.0);

    int getId() const;
    const std::string& getName() const;
    const std::string& getCategory() const;
    const std::string& getSku() const;
    double getWeight() const;
    double getVolume() const;

    void setName(const std::string& name);
    void setCategory(const std::string& category);
    void setSku(const std::string& sku);
    void setWeight(double weight);
    void setVolume(double volume);
};
