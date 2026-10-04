#pragma once

class OrderItem {
private:
    int productId_{};
    int quantity_{};

public:
    OrderItem() = default;
    OrderItem(int productId, int quantity);

    int getProductId() const;
    int getQuantity() const;

    void setQuantity(int quantity);
};
