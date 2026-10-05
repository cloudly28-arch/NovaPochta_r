#pragma once

#include <string>

class Store {
private:
    int id_{};
    std::string name_;
    std::string address_;
    std::string contactPhone_;

public:
    Store() = default;
    Store(int id,
          std::string name,
          std::string address = {},
          std::string contactPhone = {});

    int getId() const;
    const std::string& getName() const;
    const std::string& getAddress() const;
    const std::string& getContactPhone() const;

    void setName(const std::string& name);
    void setAddress(const std::string& address);
    void setContactPhone(const std::string& contactPhone);
};
