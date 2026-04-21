#pragma once

#include "database/order/order_dao.h"

#include <memory>
#include <vector>
#include <zel/myorm.h>

class OrderMysqlDao : public OrderDao {

  public:
    OrderMysqlDao(const std::shared_ptr<zel::myorm::Database> &db);
    virtual ~OrderMysqlDao() = default;

    /// @brief Add a new order to the database.
    bool add(const std::shared_ptr<Order> &order) override;

    /// @brief Remove an order from the database by its id.
    bool remove(const int &id) override;

    /// @brief Clear all orders from the database.
    bool clear() override;

    /// @brief Update an order in the database by its id.
    bool update(const int &id, const std::shared_ptr<Order> &order) override;

    /// @brief Get all orders from the database.
    std::vector<std::shared_ptr<Order>> all() override;

    /// @brief Get an order by its id or name.
    std::shared_ptr<Order> get(const int &id) override;
    std::shared_ptr<Order> get(const std::string &name) override;

    /// @brief QC confirm an order.
    bool confirm(const std::string &order_name, const std::string &confirm_by, Type type) override;

    /// @brief Check if an order with the given name exists in the database.
    bool exists(const std::string &name) override;

    /// @brief Set the current order.
    bool currentOrder(const int &order_id) override;

    /// @brief Get the current order.
    std::shared_ptr<Order> currentOrder() override;

  private:
    /// @brief Initialize the data and create tables.
    void init();

  private:
    std::shared_ptr<zel::myorm::Database> db_;
    int                                   current_order_id_;
};