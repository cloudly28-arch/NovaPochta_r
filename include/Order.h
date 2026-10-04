#pragma once

#include <string>
#include <vector>
#include "Types.h"
#include "OrderItem.h"

class Order {
private:
    int id_{};
    int storeId_{};
    OrderStatus status_{OrderStatus::Created};
    std::string createdAt_;
    std::vector<OrderItem> items_;

public:
    Order() = default;
    Order(int id, int storeId, const std::string& createdAt);

    int getId() const;
    int getStoreId() const;
    OrderStatus getStatus() const;
    const std::string& getCreatedAt() const;
    const std::vector<OrderItem>& getItems() const;

    void setStatus(OrderStatus status);
    void addItem(const OrderItem& item);
    bool removeItemByProductId(int productId);
    int getTotalItemCount() const;
};
