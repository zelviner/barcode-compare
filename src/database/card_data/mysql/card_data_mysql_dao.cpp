#include "card_data_mysql_dao.h"
#include "card_tables.hpp"
#include "utils/utils.h"

#include <memory>
#include <qsettings>
#include <vector>

CardDataMysqlDao::CardDataMysqlDao(const std::string &order_name)
    : order_name_(order_name) {
    init();
}

bool CardDataMysqlDao::batchAdd(const std::vector<std::shared_ptr<CardData>> &card_datas, const size_t batch_size) {
    std::vector<CardTables> rows;
    rows.reserve(card_datas.size());

    for (auto card_data : card_datas) {
        CardTables row;
        row["card_number"]   = card_data->card_number;
        row["iccid"]         = card_data->iccid;
        row["imsi"]          = card_data->imsi;
        row["quantity"]      = card_data->quantity;
        row["iccid_barcode"] = card_data->iccid_barcode;
        row["imsi_barcode"]  = card_data->imsi_barcode;

        rows.emplace_back(std::move(row));

        // 及时释放内存，防止内存泄漏
        if (rows.size() == batch_size) {
            CardTables(*db_, order_name_, "id").insert(rows);
            rows.clear();
        }
    }

    if (!rows.empty()) {
        CardTables(*db_, order_name_, "id").insert(rows);
    }

    return true;
}

std::vector<std::shared_ptr<CardData>> CardDataMysqlDao::all(const int &status) {
    std::vector<std::shared_ptr<CardData>> card_datas;

    std::vector<CardTables> card_tables_all;
    if (status == -1) {
        card_tables_all = CardTables(*db_, order_name_, "id").all();
    } else {
        card_tables_all = CardTables(*db_, order_name_, "id").where("status", status).all();
    }

    for (auto one : card_tables_all) {
        auto card_data           = std::make_shared<CardData>();
        card_data->id            = one("id").asInt();
        card_data->card_number   = one("card_number").asString();
        card_data->iccid         = one("iccid").asString();
        card_data->imsi          = one("imsi").asString();
        card_data->quantity      = one("quantity").asInt();
        card_data->iccid_barcode = one("iccid_barcode").asString();
        card_data->imsi_barcode  = one("imsi_barcode").asString();
        card_data->status        = one("status").asInt();
        card_data->scanned_by    = one("scanned_by").asString();
        card_data->scanned_at    = one("scanned_at").asString();

        card_datas.push_back(card_data);
    }

    return card_datas;
}

std::vector<std::shared_ptr<CardData>> CardDataMysqlDao::all(const std::string &start_number, const std::string &end_number) {
    std::vector<std::shared_ptr<CardData>> card_datas;

    std::vector<CardTables> card_tables_all =
        CardTables(*db_, order_name_, "id").where("iccid_barcode", ">=", start_number).where("iccid_barcode", "<=", end_number).all();
    for (auto one : card_tables_all) {
        auto card_data           = std::make_shared<CardData>();
        card_data->id            = one("id").asInt();
        card_data->card_number   = one("card_number").asString();
        card_data->iccid         = one("iccid").asString();
        card_data->imsi          = one("imsi").asString();
        card_data->quantity      = one("quantity").asInt();
        card_data->iccid_barcode = one("iccid_barcode").asString();
        card_data->imsi_barcode  = one("imsi_barcode").asString();
        card_data->status        = one("status").asInt();
        card_data->scanned_by    = one("scanned_by").asString();
        card_data->scanned_at    = one("scanned_at").asString();

        card_datas.push_back(card_data);
    }

    return card_datas;
}

bool CardDataMysqlDao::scanned(const std::string &start_barcode, const std::string &scanned_by) {
    auto card_data        = get(start_barcode);
    card_data->status     = 1;
    card_data->scanned_by = scanned_by;
    card_data->scanned_at = utils::Utils::now();
    return update(card_data->id, card_data);
}

bool CardDataMysqlDao::rescanned(const std::string &start_barcode, const std::string &end_barcode) {
    CardTables(*db_, order_name_, "id")
        .where("iccid_barcode", ">=", start_barcode)
        .where("iccid_barcode", "<=", end_barcode)
        .update({
            {"status", 0},
        });

    return true;
}

std::shared_ptr<CardData> CardDataMysqlDao::get(const std::string &start_barcode) {
    CardTables card_table    = CardTables(*db_, order_name_, "id").where("iccid_barcode", start_barcode).one();
    auto       card_data     = std::make_shared<CardData>();
    card_data->id            = card_table("id").asInt();
    card_data->card_number   = card_table("card_number").asString();
    card_data->iccid         = card_table("iccid").asString();
    card_data->imsi          = card_table("imsi").asString();
    card_data->quantity      = card_table("quantity").asInt();
    card_data->iccid_barcode = card_table("iccid_barcode").asString();
    card_data->imsi_barcode  = card_table("imsi_barcode").asString();
    card_data->status        = card_table("status").asInt();
    card_data->scanned_by    = card_table("scanned_by").asString();
    card_data->scanned_at    = card_table("scanned_at").asString();

    if (card_data->id == 0) {
        return nullptr;
    }

    return card_data;
}

bool CardDataMysqlDao::update(const int &id, std::shared_ptr<CardData> &card_data) {
    CardTables(*db_, order_name_, "id")
        .where("id", id)
        .update({
            {"card_number", card_data->card_number},
            {"iccid", card_data->iccid},
            {"imsi", card_data->imsi},
            {"quantity", card_data->quantity},
            {"iccid_barcode", card_data->iccid_barcode},
            {"imsi_barcode", card_data->imsi_barcode},
            {"status", card_data->status},
            {"scanned_by", card_data->scanned_by},
            {"scanned_at", card_data->scanned_at},
        });

    return true;
}

bool CardDataMysqlDao::clear() {
    std::string sql = "DROP TABLE  IF EXISTS `" + order_name_ + "`";
    db_->execute(sql);

    return true;
}

void CardDataMysqlDao::init() {
    // 读取配置文件
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("mysql");
    std::string host     = settings.value("host").toString().toStdString();
    int         port     = settings.value("port").toInt();
    std::string user     = settings.value("user").toString().toStdString();
    std::string password = settings.value("password").toString().toStdString();
    std::string database = "card_data";
    settings.endGroup();

    db_ = std::make_shared<zel::myorm::Database>();
    if (!db_->connect(host, port, user, password, database)) {
        return;
    }

    if (!CardTables(*db_, order_name_, "id").exists()) {
        std::string sql = "CREATE TABLE IF NOT EXISTS  `" + order_name_ +
            "` ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "card_number VARCHAR(255) NOT NULL,"
            "iccid VARCHAR(255) NOT NULL,"
            "imsi VARCHAR(255) NOT NULL,"
            "quantity INT NOT NULL,"
            "iccid_barcode VARCHAR(255) NOT NULL,"
            "imsi_barcode VARCHAR(255) NOT NULL,"
            "status INT NOT NULL DEFAULT 0,"
            "scanned_by VARCHAR(255) DEFAULT NULL,"
            "scanned_at VARCHAR(255) DEFAULT NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;";

        db_->execute(sql);
    }
}