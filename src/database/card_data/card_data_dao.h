#pragma once

#include "card_data.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <vector>

class CardDataDao {
  public:
    /// @brief Batch insert new card datas into the database.
    virtual bool batchAdd(const std::vector<std::shared_ptr<CardData>> &card_datas, const size_t batch_size = 20000) = 0;

    /// @brief Get all card datas from the database.
    virtual std::vector<std::shared_ptr<CardData>> all(const int &status = -1)                                         = 0;
    virtual std::vector<std::shared_ptr<CardData>> all(const std::string &start_number, const std::string &end_number) = 0;

    /// @brief Set the status of the card data to 1 when it is scanned.
    virtual bool scanned(const std::string &iccid_barcode, const std::string &scanned_by) = 0;

    /// @brief Set the status of the card data to 0 when it is rescanned.
    virtual bool rescanned(const std::string &start_barcode, const std::string &end_barcode) = 0;

    /// @brief Get card data from the database by iccid barcode.
    virtual std::shared_ptr<CardData> get(const std::string &iccid_barcode) = 0;

    /// @brief Update card data in the database.
    virtual bool update(const int &id, std::shared_ptr<CardData> &card_data) = 0;

    /// @brief Clear all data in the database.
    virtual bool clear() = 0;
};