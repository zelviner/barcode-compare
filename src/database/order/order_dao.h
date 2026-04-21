#pragma once

#include "database/format/format.h"
#include "order.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <unordered_set>
#include <vector>

class OrderDao {
  public:
    enum Type {
        CARD = 0,
        BOX,
        CARTON,
    };

  public:
    /// @brief Add a new order to the database.
    virtual bool add(const std::shared_ptr<Order> &order) = 0;

    /// @brief Remove an order from the database by its id.
    virtual bool remove(const int &id) = 0;

    /// @brief Clear all orders from the database.
    virtual bool clear() = 0;

    /// @brief Update an order in the database by its id.
    virtual bool update(const int &id, const std::shared_ptr<Order> &order) = 0;

    /// @brief Get all orders from the database.
    virtual std::vector<std::shared_ptr<Order>> all() = 0;

    /// @brief Get an order by its id or name.
    virtual std::shared_ptr<Order> get(const int &id)           = 0;
    virtual std::shared_ptr<Order> get(const std::string &name) = 0;

    /// @brief QC confirm an order.
    virtual bool confirm(const std::string &order_name, const std::string &confirm_by, Type type) = 0;

    /// @brief Check if an order with the given name exists in the database.
    virtual bool exists(const std::string &name) = 0;

    /// @brief Set the current order.
    virtual bool currentOrder(const int &order_id) = 0;

    /// @brief Get the current order.
    virtual std::shared_ptr<Order> currentOrder() = 0;

  protected:
    bool hasRequiredValues(const std::vector<std::string> &headers, const std::shared_ptr<Format> &format, bool with_barcode = true) {
        if (headers.empty()) {
            return false;
        }

        std::unordered_set<std::string> headerSet(headers.begin(), headers.end());

        if (with_barcode) {
            return headerSet.count(format->box_number) && headerSet.count(format->start_number) && headerSet.count(format->end_number) &&
                headerSet.count(format->quantity) && headerSet.count(format->barcode);
        } else {
            return headerSet.count(format->box_number) && headerSet.count(format->start_number) && headerSet.count(format->end_number) &&
                headerSet.count(format->quantity);
        }
    }
};