#pragma once

#include "database/card_data/card_data_dao.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <vector>

class CardDataSqliteDao : public CardDataDao {
  public:
    CardDataSqliteDao(const std::shared_ptr<SQLite::Database> &db, const std::string &order_name);
    virtual ~CardDataSqliteDao() = default;

    /// @brief Batch insert new card datas into the database.
    bool batchAdd(const std::vector<std::shared_ptr<CardData>> &card_datas, const size_t batch_size = 20000) override;

    /// @brief Get all card datas from the database.
    std::vector<std::shared_ptr<CardData>> all(const int &status = -1) override;
    std::vector<std::shared_ptr<CardData>> all(const std::string &start_number, const std::string &end_number) override;

    /// @brief Set the status of the card data to 1 when it is scanned.
    bool scanned(const std::string &iccid_barcode, const std::string &scanned_by) override;

    /// @brief Set the status of the card data to 0 when it is rescanned.
    bool rescanned(const std::string &start_barcode, const std::string &end_barcode) override;

    /// @brief Get card data from the database by iccid barcode.
    std::shared_ptr<CardData> get(const std::string &iccid_barcode) override;

    /// @brief Update card data in the database.
    bool update(const int &id, std::shared_ptr<CardData> &card_data) override;

    /// @brief Clear all data in the database.
    bool clear() override;

  private:
    /// @brief Initialize the data and create tables.
    void init();

  private:
    std::shared_ptr<SQLite::Database> db_;
    std::string                       order_name_;
};