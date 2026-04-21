#pragma once

#include "box_data.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <vector>

class BoxDataDao {
  public:
    enum Type {
        CARD = 0,
        BOX,
        CARTON,
    };

  public:
    /// @brief Batch insert new box datas into the database.
    virtual bool batchAdd(const std::vector<std::shared_ptr<BoxData>> &box_datas, const size_t batch_size = 20000) = 0;

    /// @brief Get all box datas from the database.
    virtual std::vector<std::shared_ptr<BoxData>> all(Type type = BOX, const int &status = -1)                        = 0;
    virtual std::vector<std::shared_ptr<BoxData>> all(const std::string &start_number, const std::string &end_number) = 0;

    /// @brief Set the status of the box data to 1 when it is scanned.
    virtual bool scanned(Type type, const std::string &start_barcode, const std::string &scanned_by) = 0;

    /// @brief Set the status of the box data to 0 when it is rescanned.
    virtual bool rescanned(Type type, const std::string &start_barcode) = 0;

    /// @brief Get box data from the database by start or end barcode.
    virtual std::shared_ptr<BoxData> get(const std::string &start_or_end_barcode) = 0;

    /// @brief Update box data in the database.
    virtual bool update(const int &id, std::shared_ptr<BoxData> &box_data) = 0;

    /// @brief Clear all data in the database.
    virtual bool clear() = 0;
};