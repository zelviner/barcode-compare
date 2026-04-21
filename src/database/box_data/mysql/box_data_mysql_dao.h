#pragma once

#include "database/box_data/box_data_dao.h"

#include <memory>
#include <vector>
#include <zel/myorm.h>

class BoxDataMysqlDao : public BoxDataDao {

  public:
    BoxDataMysqlDao(const std::string &order_name);
    virtual ~BoxDataMysqlDao() = default;

    /// @brief Batch insert new box datas into the database.
    bool batchAdd(const std::vector<std::shared_ptr<BoxData>> &box_datas, const size_t batch_size = 20000) override;

    /// @brief Get all box datas from the database.
    std::vector<std::shared_ptr<BoxData>> all(Type type = BOX, const int &status = -1) override;
    std::vector<std::shared_ptr<BoxData>> all(const std::string &start_number, const std::string &end_number) override;

    /// @brief Set the status of the box data to 1 when it is scanned.
    bool scanned(Type type, const std::string &start_barcode, const std::string &scanned_by) override;

    /// @brief Set the status of the box data to 0 when it is rescanned.
    bool rescanned(Type type, const std::string &start_barcode) override;

    /// @brief Get box data from the database by start or end barcode.
    std::shared_ptr<BoxData> get(const std::string &start_or_end_barcode) override;

    /// @brief Update box data in the database.
    bool update(const int &id, std::shared_ptr<BoxData> &box_data) override;

    /// @brief Clear all data in the database.
    bool clear() override;

  private:
    /// @brief Initialize the data and create tables.
    void init();

  private:
    std::shared_ptr<zel::myorm::Database> db_;
    std::string                           order_name_;
};