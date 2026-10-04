#pragma once

class InventoryItem {
private:
    int productId_{};
    int storageCellId_{};
    int quantity_{};

public:
    InventoryItem() = default;
    InventoryItem(int productId, int storageCellId, int quantity);

    int getProductId() const;
    int getStorageCellId() const;
    int getQuantity() const;

    void setQuantity(int quantity);
    void addQuantity(int quantity);
    bool removeQuantity(int quantity);
};
