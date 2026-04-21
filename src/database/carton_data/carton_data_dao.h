#pragma once

#include "carton_data.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <vector>

class CartonDataDao {
  public:
    /// @brief Batch insert new box datas into the database.
    virtual bool batchAdd(const std::vector<std::shared_ptr<CartonData>> &carton_datas, const size_t batch_size = 20000) = 0;

    /// @brief Get all carton datas from the database.
    virtual std::vector<std::shared_ptr<CartonData>> all(const int &status = -1) = 0;

    /// @brief Set the status of the carton data to 1 when it is scanned.
    virtual bool scanned(const std::string &start_barcode, const std::string &scanned_by) = 0;

    /// @brief Get carton data from the database by start or end barcode.
    virtual std::shared_ptr<CartonData> get(const std::string &start_or_end_barcode) = 0;

    /// @brief Update carton data in the database.
    virtual bool update(const int &id, std::shared_ptr<CartonData> &carton_data) = 0;

    /// @brief Clear all data in the database.
    virtual bool clear() = 0;
};