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
    std::string unitName_{"шт"};
    int unitsPerPackage_{1};
    double pricePerUnit_{0.0};
    int shelfLifeDays_{0};

public:
    Product() = default;
    Product(int id,
            std::string name,
            std::string category = {},
            std::string sku = {},
            double weight = 0.0,
            double volume = 0.0,
            std::string unitName = "шт",
            int unitsPerPackage = 1,
            double pricePerUnit = 0.0,
            int shelfLifeDays = 0);

    int getId() const;
    const std::string& getName() const;
    const std::string& getCategory() const;
    const std::string& getSku() const;
    double getWeight() const;
    double getVolume() const;
    const std::string& getUnitName() const;
    int getUnitsPerPackage() const;
    double getPricePerUnit() const;
    int getShelfLifeDays() const;

    void setName(const std::string& name);
    void setCategory(const std::string& category);
    void setSku(const std::string& sku);
    void setWeight(double weight);
    void setVolume(double volume);
};
