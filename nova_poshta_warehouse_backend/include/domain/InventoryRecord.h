#pragma once

#include <string>

class InventoryRecord {
private:
    int id_{};
    int productId_{};
    int storageCellId_{-1};
    int quantity_{};
    int reservedQuantity_{};
    std::string batchNumber_;
    std::string expirationDate_; // YYYY-MM-DD, empty for non-expiring goods

public:
    InventoryRecord() = default;
    InventoryRecord(int id,
                    int productId,
                    int storageCellId,
                    int quantity,
                    int reservedQuantity = 0,
                    std::string batchNumber = {},
                    std::string expirationDate = {});

    int getId() const;
    int getProductId() const;
    int getStorageCellId() const;
    int getQuantity() const;
    int getReservedQuantity() const;
    int getAvailableQuantity() const;
    const std::string& getBatchNumber() const;
    const std::string& getExpirationDate() const;

    void addQuantity(int quantity);
    bool removeQuantity(int quantity);
    bool reserve(int quantity);
    bool releaseReservation(int quantity);
    bool shipReserved(int quantity);
};
