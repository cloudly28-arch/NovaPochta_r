#include "Store.h"

#include <sstream>
#include <stdexcept>
#include <utility>

Store::Store(int id, std::string name, std::string address, std::string contactPhone)
    : id_(id),
      name_(std::move(name)),
      address_(std::move(address)),
      contactPhone_(std::move(contactPhone)) {
    if (id_ <= 0) {
        throw std::invalid_argument("Store id must be positive");
    }
    if (name_.empty()) {
        throw std::invalid_argument("Store name cannot be empty");
    }
}

int Store::getId() const { return id_; }
const std::string& Store::getName() const { return name_; }
const std::string& Store::getAddress() const { return address_; }
const std::string& Store::getContactPhone() const { return contactPhone_; }

void Store::setName(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Store name cannot be empty");
    }
    name_ = name;
}

void Store::setAddress(const std::string& address) { address_ = address; }
void Store::setContactPhone(const std::string& contactPhone) { contactPhone_ = contactPhone; }

std::string Store::toString() const {
    std::ostringstream out;
    out << "Store{id=" << id_
        << ", name='" << name_ << "'"
        << ", address='" << address_ << "'"
        << ", phone='" << contactPhone_ << "'"
        << "}";
    return out.str();
}
