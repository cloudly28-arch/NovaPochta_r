#include "domain/Product.h"
#include <stdexcept>
#include <utility>

Product::Product(int id, std::string name, std::string category, std::string sku,
                 double weight, double volume, std::string unitName,
                 int unitsPerPackage, double pricePerUnit, int shelfLifeDays)
    : id_(id), name_(std::move(name)), category_(std::move(category)),
      sku_(std::move(sku)), weight_(weight), volume_(volume),
      unitName_(std::move(unitName)), unitsPerPackage_(unitsPerPackage),
      pricePerUnit_(pricePerUnit), shelfLifeDays_(shelfLifeDays) {
    if (id_ <= 0) throw std::invalid_argument("Product id must be positive");
    if (name_.empty()) throw std::invalid_argument("Product name cannot be empty");
    if (weight_ < 0 || volume_ < 0) throw std::invalid_argument("Weight and volume cannot be negative");
    if (unitName_.empty()) throw std::invalid_argument("Unit name cannot be empty");
    if (unitsPerPackage_ <= 0) throw std::invalid_argument("Units per package must be positive");
    if (pricePerUnit_ < 0) throw std::invalid_argument("Price cannot be negative");
    if (shelfLifeDays_ < 0) throw std::invalid_argument("Shelf life cannot be negative");
}
int Product::getId() const { return id_; }
const std::string& Product::getName() const { return name_; }
const std::string& Product::getCategory() const { return category_; }
const std::string& Product::getSku() const { return sku_; }
double Product::getWeight() const { return weight_; }
double Product::getVolume() const { return volume_; }
const std::string& Product::getUnitName() const { return unitName_; }
int Product::getUnitsPerPackage() const { return unitsPerPackage_; }
double Product::getPricePerUnit() const { return pricePerUnit_; }
int Product::getShelfLifeDays() const { return shelfLifeDays_; }
void Product::setName(const std::string& name) { if (name.empty()) throw std::invalid_argument("Product name cannot be empty"); name_ = name; }
void Product::setCategory(const std::string& category) { category_ = category; }
void Product::setSku(const std::string& sku) { sku_ = sku; }
void Product::setWeight(double weight) { if (weight < 0) throw std::invalid_argument("Weight cannot be negative"); weight_ = weight; }
void Product::setVolume(double volume) { if (volume < 0) throw std::invalid_argument("Volume cannot be negative"); volume_ = volume; }
