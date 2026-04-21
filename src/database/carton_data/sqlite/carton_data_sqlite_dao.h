#pragma once

#include "database/carton_data/carton_data_dao.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <vector>

class CartonDataSqliteDao : public CartonDataDao {
  public:
    CartonDataSqliteDao(const std::shared_ptr<SQLite::Database> &db, const std::string &order_name);
    virtual ~CartonDataSqliteDao() = default;

    /// @brief Batch insert new box datas into the database.
    bool batchAdd(const std::vector<std::shared_ptr<CartonData>> &carton_datas, const size_t batch_size = 20000) override;

    /// @brief Get all carton datas from the database.
    std::vector<std::shared_ptr<CartonData>> all(const int &status = -1) override;

    /// @brief Set the status of the carton data to 1 when it is scanned.
    bool scanned(const std::string &start_barcode, const std::string &scanned_by) override;

    /// @brief Get carton data from the database by start or end barcode.
    std::shared_ptr<CartonData> get(const std::string &start_or_end_barcode) override;

    /// @brief Update carton data in the database.
    bool update(const int &id, std::shared_ptr<CartonData> &carton_data) override;

    /// @brief Clear all data in the database.
    bool clear() override;

  private:
    /// @brief Initialize the data and create tables.
    void init();

  private:
    std::shared_ptr<SQLite::Database> db_;
    std::string                       order_name_;
};