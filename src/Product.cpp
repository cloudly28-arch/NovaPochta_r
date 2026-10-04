#include "Product.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

Product::Product(int id, std::string sku, std::string name, double unitWeightKg, double unitPrice)
    : id_(id),
      sku_(std::move(sku)),
      name_(std::move(name)),
      unitWeightKg_(unitWeightKg),
      unitPrice_(unitPrice) {
    if (id_ <= 0) throw std::invalid_argument("Product id must be positive");
    if (sku_.empty()) throw std::invalid_argument("SKU cannot be empty");
    if (name_.empty()) throw std::invalid_argument("Product name cannot be empty");
    if (unitWeightKg_ < 0) throw std::invalid_argument("Weight cannot be negative");
    if (unitPrice_ < 0) throw std::invalid_argument("Price cannot be negative");
}

int Product::getId() const { return id_; }
const std::string& Product::getSku() const { return sku_; }
const std::string& Product::getName() const { return name_; }
double Product::getUnitWeightKg() const { return unitWeightKg_; }
double Product::getUnitPrice() const { return unitPrice_; }

void Product::setName(const std::string& name) {
    if (name.empty()) throw std::invalid_argument("Product name cannot be empty");
    name_ = name;
}

void Product::setUnitWeightKg(double weightKg) {
    if (weightKg < 0) throw std::invalid_argument("Weight cannot be negative");
    unitWeightKg_ = weightKg;
}

void Product::setUnitPrice(double price) {
    if (price < 0) throw std::invalid_argument("Price cannot be negative");
    unitPrice_ = price;
}

std::string Product::toString() const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2)
        << "Product{id=" << id_
        << ", sku='" << sku_ << "'"
        << ", name='" << name_ << "'"
        << ", weightKg=" << unitWeightKg_
        << ", price=" << unitPrice_
        << "}";
    return out.str();
}
