#include "card_data_sqlite_dao.h"
#include "utils/utils.h"

#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>
#include <zel/core.h>

CardDataSqliteDao::CardDataSqliteDao(const std::shared_ptr<SQLite::Database> &db, const std::string &order_name)
    : db_(db)
    , order_name_(order_name) {
    init();
}

bool CardDataSqliteDao::batchAdd(const std::vector<std::shared_ptr<CardData>> &card_datas, const size_t batch_size) {
    try {
        SQLite::Transaction transaction(*db_);
        SQLite::Statement   query(
            *db_, "INSERT INTO card_data.[" + order_name_ + "] (card_number, iccid, imsi, quantity, iccid_barcode, imsi_barcode) VALUES (?,?,?,?,?,?)");
        for (const std::shared_ptr<CardData> &card_data : card_datas) {
            query.bind(1, card_data->card_number);
            query.bind(2, card_data->iccid);
            query.bind(3, card_data->imsi);
            query.bind(4, card_data->quantity);
            query.bind(5, card_data->iccid_barcode);
            query.bind(6, card_data->imsi_barcode);
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

std::vector<std::shared_ptr<CardData>> CardDataSqliteDao::all(const int &status) {
    std::string sql;
    if (status == -1) {
        sql = "SELECT * FROM card_data.[" + order_name_ + "]";
    } else {
        sql = "SELECT * FROM card_data.[" + order_name_ + "] WHERE status = ?";
    }

    SQLite::Statement all(*db_, sql);
    if (status != -1) {
        all.bind(1, status);
    }

    std::vector<std::shared_ptr<CardData>> card_datas;
    while (all.executeStep()) {
        std::shared_ptr<CardData> card_data = std::make_shared<CardData>();
        card_data->id                       = all.getColumn("id");
        card_data->card_number              = all.getColumn("card_number").getString();
        card_data->iccid                    = all.getColumn("iccid").getString();
        card_data->imsi                     = all.getColumn("imsi").getString();
        card_data->quantity                 = all.getColumn("quantity").getInt();
        card_data->iccid_barcode            = all.getColumn("iccid_barcode").getString();
        card_data->imsi_barcode             = all.getColumn("imsi_barcode").getString();
        card_data->status                   = all.getColumn("status");
        card_data->scanned_by               = all.getColumn("scanned_by").getString();
        card_data->scanned_at               = all.getColumn("scanned_at").getString();

        card_datas.push_back(card_data);
    }

    return card_datas;
}

std::vector<std::shared_ptr<CardData>> CardDataSqliteDao::all(const std::string &start_number, const std::string &end_number) {
    std::string       sql = "SELECT * FROM card_data.[" + order_name_ + "] WHERE iccid_barcode >= ? AND iccid_barcode <= ?";
    SQLite::Statement all(*db_, sql);
    all.bind(1, start_number);
    all.bind(2, end_number);

    std::vector<std::shared_ptr<CardData>> card_datas;
    while (all.executeStep()) {
        std::shared_ptr<CardData> card_data = std::make_shared<CardData>();
        card_data->id                       = all.getColumn("id");
        card_data->card_number              = all.getColumn("card_number").getString();
        card_data->iccid                    = all.getColumn("iccid").getString();
        card_data->imsi                     = all.getColumn("imsi").getString();
        card_data->quantity                 = all.getColumn("quantity").getInt();
        card_data->iccid_barcode            = all.getColumn("iccid_barcode").getString();
        card_data->imsi_barcode             = all.getColumn("imsi_barcode").getString();
        card_data->status                   = all.getColumn("status");
        card_data->scanned_by               = all.getColumn("scanned_by").getString();
        card_data->scanned_at               = all.getColumn("scanned_at").getString();

        card_datas.push_back(card_data);
    }

    return card_datas;
}

bool CardDataSqliteDao::scanned(const std::string &start_barcode, const std::string &scanned_by) {
    auto card_data        = get(start_barcode);
    card_data->status     = 1;
    card_data->scanned_by = scanned_by;
    card_data->scanned_at = utils::Utils::now();
    return update(card_data->id, card_data);
}

bool CardDataSqliteDao::rescanned(const std::string &start_barcode, const std::string &end_barcode) {
    std::string       sql = "UPDATE card_data.[" + order_name_ + "] SET status = 0 WHERE iccid_barcode >= ? AND iccid_barcode <= ?";
    SQLite::Statement update(*db_, sql);
    update.bind(1, start_barcode);
    update.bind(2, end_barcode);

    return update.exec();
}

std::shared_ptr<CardData> CardDataSqliteDao::get(const std::string &start_barcode) {
    std::string       sql = "SELECT * FROM card_data.[" + order_name_ + "] WHERE iccid_barcode = ?";
    SQLite::Statement get(*db_, sql);
    get.bind(1, start_barcode);

    if (get.executeStep()) {
        std::shared_ptr<CardData> card_data = std::make_shared<CardData>();
        card_data->id                       = get.getColumn("id");
        card_data->card_number              = get.getColumn("card_number").getString();
        card_data->iccid                    = get.getColumn("iccid").getString();
        card_data->imsi                     = get.getColumn("imsi").getString();
        card_data->quantity                 = get.getColumn("quantity").getInt();
        card_data->iccid_barcode            = get.getColumn("iccid_barcode").getString();
        card_data->imsi_barcode             = get.getColumn("imsi_barcode").getString();
        card_data->status                   = get.getColumn("status");
        card_data->scanned_by               = get.getColumn("scanned_by").getString();
        card_data->scanned_at               = get.getColumn("scanned_at").getString();

        return card_data;
    }

    return nullptr;
}

bool CardDataSqliteDao::update(const int &id, std::shared_ptr<CardData> &card_data) {
    try {
        std::string sql = "UPDATE card_data.[" + order_name_ +
            "] SET card_number = ?, iccid = ?, imsi = ?,  quantity = ?, iccid_barcode = ?, imsi_barcode = ?, status = ?, scanned_by = ?, scanned_at = ? WHERE "
            "id = ?";
        SQLite::Statement update(*db_, sql);
        update.bind(1, card_data->card_number);
        update.bind(2, card_data->iccid);
        update.bind(3, card_data->imsi);
        update.bind(4, card_data->quantity);
        update.bind(5, card_data->iccid_barcode);
        update.bind(6, card_data->imsi_barcode);
        update.bind(7, card_data->status);
        update.bind(8, card_data->scanned_by);
        update.bind(9, card_data->scanned_at);
        update.bind(10, id);

        return update.exec();
    } catch (std::exception &e) {
        log_error("Update card data failed: %s", e.what());
        return false;
    }

    return true;
}

bool CardDataSqliteDao::clear() {
    std::string sql = "DROP TABLE IF EXISTS card_data.[" + order_name_ + "]";
    return db_->exec(sql);
}

void CardDataSqliteDao::init() {
    std::string       sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'card_data." + order_name_ + "'";
    SQLite::Statement query(*db_, sql);

    if (!query.executeStep()) {
        sql = "CREATE TABLE IF NOT EXISTS card_data.[" + order_name_ +
            "] (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "card_number TEXT,"
            "iccid TEXT,"
            "imsi TEXT,"
            "quantity INTEGER,"
            "iccid_barcode TEXT,"
            "imsi_barcode TEXT,"
            "status INTEGER DEFAULT 0,"
            "scanned_by TEXT DEFAULT '',"
            "scanned_at TEXT DEFAULT '')";

        db_->exec(sql);
    }
}