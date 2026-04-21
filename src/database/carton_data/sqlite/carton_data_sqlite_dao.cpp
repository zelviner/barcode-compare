#include "carton_data_sqlite_dao.h"
#include "utils/utils.h"

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

CartonDataSqliteDao::CartonDataSqliteDao(const std::shared_ptr<SQLite::Database> &db, const std::string &order_name)
    : db_(db)
    , order_name_(order_name) {
    init();
}

bool CartonDataSqliteDao::batchAdd(const std::vector<std::shared_ptr<CartonData>> &carton_datas, const size_t batch_size) {
    try {
        SQLite::Transaction transaction(*db_);
        SQLite::Statement   query(*db_,
                                  "INSERT INTO carton_data.[" + order_name_ +
                                      "] (filename, carton_number, start_number, end_number, quantity, start_barcode, end_barcode) VALUES (?,?,?,?,?,?,?)");
        for (const std::shared_ptr<CartonData> &carton_data : carton_datas) {
            query.bind(1, carton_data->filename);
            query.bind(2, carton_data->carton_number);
            query.bind(3, carton_data->start_number);
            query.bind(4, carton_data->end_number);
            query.bind(5, carton_data->quantity);
            query.bind(6, carton_data->start_barcode);
            query.bind(7, carton_data->end_barcode);
            query.exec();

            query.reset();
        }

        transaction.commit();
    } catch (std::exception &e) {
        std::cerr << "Batch insert failed: " << e.what() << std::endl;
        throw;
        return false;
    }

    return true;
}

std::vector<std::shared_ptr<CartonData>> CartonDataSqliteDao::all(const int &status) {
    std::string sql;
    if (status == -1) {
        sql = "SELECT * FROM carton_data.[" + order_name_ + "]";
    } else {
        sql = "SELECT * FROM carton_data.[" + order_name_ + "] WHERE status = ?";
    }

    SQLite::Statement all(*db_, sql);
    if (status != -1) {
        all.bind(1, status);
    }

    std::vector<std::shared_ptr<CartonData>> carton_datas;
    while (all.executeStep()) {
        std::shared_ptr<CartonData> carton_data = std::make_shared<CartonData>();
        carton_data->id                         = all.getColumn("id");
        carton_data->filename                   = all.getColumn("filename").getString();
        carton_data->carton_number              = all.getColumn("carton_number").getString();
        carton_data->start_number               = all.getColumn("start_number").getString();
        carton_data->end_number                 = all.getColumn("end_number").getString();
        carton_data->quantity                   = all.getColumn("quantity").getInt();
        carton_data->start_barcode              = all.getColumn("start_barcode").getString();
        carton_data->end_barcode                = all.getColumn("end_barcode").getString();
        carton_data->status                     = all.getColumn("status");
        carton_data->scanned_by                 = all.getColumn("scanned_by").getString();
        carton_data->scanned_at                 = all.getColumn("scanned_at").getString();

        carton_datas.push_back(carton_data);
    }

    return carton_datas;
}

bool CartonDataSqliteDao::scanned(const std::string &start_barcode, const std::string &scanned_by) {
    auto carton_data        = get(start_barcode);
    carton_data->status     = 1;
    carton_data->scanned_by = scanned_by;
    carton_data->scanned_at = utils::Utils::now();
    return update(carton_data->id, carton_data);
}

std::shared_ptr<CartonData> CartonDataSqliteDao::get(const std::string &start_or_end_barcode) {
    std::string       sql = "SELECT * FROM carton_data.[" + order_name_ + "] WHERE start_barcode = ? OR end_barcode = ?";
    SQLite::Statement get(*db_, sql);
    get.bind(1, start_or_end_barcode);
    get.bind(2, start_or_end_barcode);

    if (get.executeStep()) {
        std::shared_ptr<CartonData> carton_data = std::make_shared<CartonData>();
        carton_data->id                         = get.getColumn("id");
        carton_data->filename                   = get.getColumn("filename").getString();
        carton_data->carton_number              = get.getColumn("carton_number").getString();
        carton_data->start_number               = get.getColumn("start_number").getString();
        carton_data->end_number                 = get.getColumn("end_number").getString();
        carton_data->quantity                   = get.getColumn("quantity").getInt();
        carton_data->start_barcode              = get.getColumn("start_barcode").getString();
        carton_data->end_barcode                = get.getColumn("end_barcode").getString();
        carton_data->status                     = get.getColumn("status");
        carton_data->scanned_by                 = get.getColumn("scanned_by").getString();
        carton_data->scanned_at                 = get.getColumn("scanned_at").getString();

        return carton_data;
    }

    return nullptr;
}

bool CartonDataSqliteDao::update(const int &id, std::shared_ptr<CartonData> &carton_data) {
    try {
        std::string sql = "UPDATE carton_data.[" + order_name_ +
            "] SET filename = ?, carton_number = ?, start_number = ?, end_number = ?, quantity = ?, start_barcode = ?, end_barcode = ?, status = ?, scanned_by "
            "= ?, scanned_at = ? WHERE id = "
            "?";
        SQLite::Statement update(*db_, sql);
        update.bind(1, carton_data->filename);
        update.bind(2, carton_data->carton_number);
        update.bind(3, carton_data->start_number);
        update.bind(4, carton_data->end_number);
        update.bind(5, carton_data->quantity);
        update.bind(6, carton_data->start_barcode);
        update.bind(7, carton_data->end_barcode);
        update.bind(8, carton_data->status);
        update.bind(9, carton_data->scanned_by);
        update.bind(10, carton_data->scanned_at);
        update.bind(11, id);

        return update.exec();
    } catch (std::exception &e) {
        std::cerr << "Update carton data failed: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool CartonDataSqliteDao::clear() {
    std::string sql = "DROP TABLE IF EXISTS carton_data.[" + order_name_ + "]";
    return db_->exec(sql);
}

void CartonDataSqliteDao::init() {
    std::string       sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'carton_data." + order_name_ + "'";
    SQLite::Statement query(*db_, sql);

    if (!query.executeStep()) {
        sql = "CREATE TABLE IF NOT EXISTS carton_data.[" + order_name_ +
            "] (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "filename TEXT,"
            "carton_number TEXT,"
            "start_number TEXT,"
            "end_number TEXT,"
            "quantity INTEGER,"
            "start_barcode TEXT,"
            "end_barcode TEXT,"
            "status INTEGER DEFAULT 0,"
            "scanned_by TEXT DEFAULT '',"
            "scanned_at TEXT DEFAULT '')";

        db_->exec(sql);
    }
}