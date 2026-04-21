#include "box_data_sqlite_dao.h"
#include "utils/utils.h"

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <exception>
#include <iostream>
#include <memory>
#include <zel/core.h>

BoxDataSqliteDao::BoxDataSqliteDao(const std::shared_ptr<SQLite::Database> &db, const std::string &order_name)
    : db_(db)
    , order_name_(order_name) {
    init();
}

bool BoxDataSqliteDao::batchAdd(const std::vector<std::shared_ptr<BoxData>> &box_datas, const size_t batch_size) {
    try {
        SQLite::Transaction transaction(*db_);
        SQLite::Statement   query(*db_,
                                  "INSERT INTO box_data.[" + order_name_ +
                                      "] (filename, box_number, start_number, end_number, quantity, start_barcode, end_barcode) VALUES (?,?,?,?,?,?,?)");
        for (const std::shared_ptr<BoxData> &box_data : box_datas) {
            query.bind(1, box_data->filename);
            query.bind(2, box_data->box_number);
            query.bind(3, box_data->start_number);
            query.bind(4, box_data->end_number);
            query.bind(5, box_data->quantity);
            query.bind(6, box_data->start_barcode);
            query.bind(7, box_data->end_barcode);
            query.exec();

            query.reset();
        }

        transaction.commit();
    } catch (std::exception &e) {
        log_error("Batch insert failed: %s", e.what());
        return false;
    }

    return true;
}

std::vector<std::shared_ptr<BoxData>> BoxDataSqliteDao::all(Type type, const int &status) {
    std::string sql;
    if (status == -1) {
        sql = "SELECT * FROM box_data.[" + order_name_ + "]";
    } else {
        switch (type) {

        case Type::CARD:
            sql = "SELECT * FROM box_data.[" + order_name_ + "] WHERE card_status = ?";
            break;

        case Type::BOX:
            sql = "SELECT * FROM box_data.[" + order_name_ + "] WHERE status = ?";
            break;

        case Type::CARTON:
            sql = "SELECT * FROM box_data.[" + order_name_ + "] WHERE carton_status = ?";
            break;
        }
    }

    std::vector<std::shared_ptr<BoxData>> box_datas;
    try {
        SQLite::Statement all(*db_, sql);
        if (status != -1) {
            all.bind(1, status);
        }

        while (all.executeStep()) {
            std::shared_ptr<BoxData> box_data = std::make_shared<BoxData>();
            box_data->id                      = all.getColumn("id");
            box_data->filename                = all.getColumn("filename").getString();
            box_data->box_number              = all.getColumn("box_number").getString();
            box_data->start_number            = all.getColumn("start_number").getString();
            box_data->end_number              = all.getColumn("end_number").getString();
            box_data->quantity                = all.getColumn("quantity").getInt();
            box_data->start_barcode           = all.getColumn("start_barcode").getString();
            box_data->end_barcode             = all.getColumn("end_barcode").getString();
            box_data->status                  = all.getColumn("status");
            box_data->card_status             = all.getColumn("card_status");
            box_data->carton_status           = all.getColumn("carton_status");
            box_data->scanned_by              = all.getColumn("scanned_by").getString();
            box_data->scanned_at              = all.getColumn("scanned_at").getString();

            box_datas.push_back(box_data);
        }
    } catch (const std::exception &e) {
        printf("SQL ERROR: %s\n", e.what());
        return box_datas;
    }

    return box_datas;
}

std::vector<std::shared_ptr<BoxData>> BoxDataSqliteDao::all(const std::string &start_number, const std::string &end_number) {
    std::string       sql = "SELECT * FROM box_data.[" + order_name_ + "] WHERE start_number >= ? AND end_number <= ?";
    SQLite::Statement all(*db_, sql);
    all.bind(1, start_number);
    all.bind(2, end_number);

    std::vector<std::shared_ptr<BoxData>> box_datas;
    while (all.executeStep()) {
        std::shared_ptr<BoxData> box_data = std::make_shared<BoxData>();
        box_data->id                      = all.getColumn("id");
        box_data->filename                = all.getColumn("filename").getString();
        box_data->box_number              = all.getColumn("box_number").getString();
        box_data->start_number            = all.getColumn("start_number").getString();
        box_data->end_number              = all.getColumn("end_number").getString();
        box_data->quantity                = all.getColumn("quantity").getInt();
        box_data->start_barcode           = all.getColumn("start_barcode").getString();
        box_data->end_barcode             = all.getColumn("end_barcode").getString();
        box_data->status                  = all.getColumn("status");
        box_data->card_status             = all.getColumn("card_status");
        box_data->carton_status           = all.getColumn("carton_status");
        box_data->scanned_by              = all.getColumn("scanned_by").getString();
        box_data->scanned_at              = all.getColumn("scanned_at").getString();

        box_datas.push_back(box_data);
    }

    return box_datas;
}

bool BoxDataSqliteDao::scanned(Type type, const std::string &start_barcode, const std::string &scanned_by) {
    auto box_data = get(start_barcode);
    switch (type) {
    case Type::CARD:
        box_data->card_status = 1;
        box_data->scanned_by  = scanned_by;
        box_data->scanned_at  = utils::Utils::now();
        break;

    case Type::BOX:
        box_data->status     = 1;
        box_data->scanned_by = scanned_by;
        box_data->scanned_at = utils::Utils::now();
        break;

    case Type::CARTON:
        box_data->carton_status = 1;
        box_data->scanned_by    = scanned_by;
        box_data->scanned_at    = utils::Utils::now();
        break;
    }

    return update(box_data->id, box_data);
}

bool BoxDataSqliteDao::rescanned(Type type, const std::string &start_barcode) {
    auto box_data = get(start_barcode);
    switch (type) {
    case Type::CARD:
        box_data->card_status = 0;
        break;

    case Type::BOX:
        box_data->status = 0;
        break;

    case Type::CARTON:
        box_data->carton_status = 0;
        break;
    }

    return update(box_data->id, box_data);
}

std::shared_ptr<BoxData> BoxDataSqliteDao::get(const std::string &start_or_end_barcode) {
    std::string       sql = "SELECT * FROM box_data.[" + order_name_ + "] WHERE start_barcode = ? OR end_barcode = ?";
    SQLite::Statement get(*db_, sql);
    get.bind(1, start_or_end_barcode);
    get.bind(2, start_or_end_barcode);

    if (get.executeStep()) {
        std::shared_ptr<BoxData> box_data = std::make_shared<BoxData>();
        box_data->id                      = get.getColumn("id");
        box_data->filename                = get.getColumn("filename").getString();
        box_data->box_number              = get.getColumn("box_number").getString();
        box_data->start_number            = get.getColumn("start_number").getString();
        box_data->end_number              = get.getColumn("end_number").getString();
        box_data->quantity                = get.getColumn("quantity").getInt();
        box_data->start_barcode           = get.getColumn("start_barcode").getString();
        box_data->end_barcode             = get.getColumn("end_barcode").getString();
        box_data->status                  = get.getColumn("status");
        box_data->card_status             = get.getColumn("card_status");
        box_data->carton_status           = get.getColumn("carton_status");
        box_data->scanned_by              = get.getColumn("scanned_by").getString();
        box_data->scanned_at              = get.getColumn("scanned_at").getString();

        return box_data;
    }

    return nullptr;
}

bool BoxDataSqliteDao::update(const int &id, std::shared_ptr<BoxData> &box_data) {
    try {
        std::string sql = "UPDATE box_data.[" + order_name_ +
            "] SET filename = ?,box_number = ?, start_number = ?, end_number = ?, quantity = ?, start_barcode = ?, end_barcode = ?, status = ?, card_status = "
            "?, carton_status = ? , scanned_by = ?, scanned_at = ? WHERE id = ?";
        SQLite::Statement update(*db_, sql);
        update.bind(1, box_data->filename);
        update.bind(2, box_data->box_number);
        update.bind(3, box_data->start_number);
        update.bind(4, box_data->end_number);
        update.bind(5, box_data->quantity);
        update.bind(6, box_data->start_barcode);
        update.bind(7, box_data->end_barcode);
        update.bind(8, box_data->status);
        update.bind(9, box_data->card_status);
        update.bind(10, box_data->carton_status);
        update.bind(11, box_data->scanned_by);
        update.bind(12, box_data->scanned_at);
        update.bind(13, id);

        return update.exec();
    } catch (std::exception &e) {
        std::cerr << "Update box data failed: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool BoxDataSqliteDao::clear() {
    std::string sql = "DROP TABLE  IF EXISTS box_data.[" + order_name_ + "]";
    return db_->exec(sql);
}

void BoxDataSqliteDao::init() {
    std::string       sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'box_data." + order_name_ + "'";
    SQLite::Statement query(*db_, sql);

    if (!query.executeStep()) {
        sql = "CREATE TABLE IF NOT EXISTS box_data.[" + order_name_ +
            "] (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "filename TEXT,"
            "box_number TEXT,"
            "start_number TEXT,"
            "end_number TEXT,"
            "quantity INTEGER,"
            "start_barcode TEXT,"
            "end_barcode TEXT,"
            "status INTEGER DEFAULT 0,"
            "card_status INTEGER DEFAULT 0,"
            "carton_status INTEGER DEFAULT 0,"
            "scanned_by TEXT DEFAULT '',"
            "scanned_at TEXT DEFAULT '')";

        db_->exec(sql);
    }
}