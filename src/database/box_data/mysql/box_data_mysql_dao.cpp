#include "box_data_mysql_dao.h"
#include "box_tables.hpp"
#include "utils/utils.h"

#include <memory>
#include <qsettings>

BoxDataMysqlDao::BoxDataMysqlDao(const std::string &order_name)
    : order_name_(order_name) {
    init();
}

bool BoxDataMysqlDao::batchAdd(const std::vector<std::shared_ptr<BoxData>> &box_datas, const size_t batch_size) {
    std::vector<BoxTables> rows;
    rows.reserve(box_datas.size());

    for (auto box_data : box_datas) {
        BoxTables row;
        row["filename"]      = box_data->filename;
        row["box_number"]    = box_data->box_number;
        row["start_number"]  = box_data->start_number;
        row["end_number"]    = box_data->end_number;
        row["quantity"]      = box_data->quantity;
        row["start_barcode"] = box_data->start_barcode;
        row["end_barcode"]   = box_data->end_barcode;

        rows.emplace_back(std::move(row));

        // 及时释放内存，防止内存泄漏
        if (rows.size() == batch_size) {
            BoxTables(*db_, order_name_, "id").insert(rows);
            rows.clear();
        }
    }

    if (!rows.empty()) {
        BoxTables(*db_, order_name_, "id").insert(rows);
    }

    return true;
}

std::vector<std::shared_ptr<BoxData>> BoxDataMysqlDao::all(Type type, const int &status) {
    std::vector<std::shared_ptr<BoxData>> box_datas;

    std::vector<BoxTables> box_tables_all;
    if (status == -1) {
        box_tables_all = BoxTables(*db_, order_name_, "id").all();
    } else {
        switch (type) {
        case Type::CARD:
            box_tables_all = BoxTables(*db_, order_name_, "id").where("card_status", status).all();
            break;

        case Type::BOX:
            box_tables_all = BoxTables(*db_, order_name_, "id").where("status", status).all();
            break;

        case Type::CARTON:
            box_tables_all = BoxTables(*db_, order_name_, "id").where("carton_status", status).all();
            break;
        }
    }

    for (auto one : box_tables_all) {
        auto box_data           = std::make_shared<BoxData>();
        box_data->id            = one("id").asInt();
        box_data->filename      = one("filename").asString();
        box_data->box_number    = one("box_number").asString();
        box_data->start_number  = one("start_number").asString();
        box_data->end_number    = one("end_number").asString();
        box_data->quantity      = one("quantity").asInt();
        box_data->start_barcode = one("start_barcode").asString();
        box_data->end_barcode   = one("end_barcode").asString();
        box_data->status        = one("status").asInt();
        box_data->card_status   = one("card_status").asInt();
        box_data->carton_status = one("carton_status").asInt();
        box_data->scanned_by    = one("scanned_by").asString();
        box_data->scanned_at    = one("scanned_at").asString();

        box_datas.push_back(box_data);
    }

    return box_datas;
}

std::vector<std::shared_ptr<BoxData>> BoxDataMysqlDao::all(const std::string &start_number, const std::string &end_number) {
    std::vector<std::shared_ptr<BoxData>> box_datas;

    std::vector<BoxTables> box_tables_all =
        BoxTables(*db_, order_name_, "id").where("start_number", ">=", start_number).where("end_number", "<=", end_number).all();
    for (auto one : box_tables_all) {
        auto box_data           = std::make_shared<BoxData>();
        box_data->id            = one("id").asInt();
        box_data->filename      = one("filename").asString();
        box_data->box_number    = one("box_number").asString();
        box_data->start_number  = one("start_number").asString();
        box_data->end_number    = one("end_number").asString();
        box_data->quantity      = one("quantity").asInt();
        box_data->start_barcode = one("start_barcode").asString();
        box_data->end_barcode   = one("end_barcode").asString();
        box_data->status        = one("status").asInt();
        box_data->card_status   = one("card_status").asInt();
        box_data->carton_status = one("carton_status").asInt();
        box_data->scanned_by    = one("scanned_by").asString();
        box_data->scanned_at    = one("scanned_at").asString();

        box_datas.push_back(box_data);
    }

    return box_datas;
}

bool BoxDataMysqlDao::scanned(Type type, const std::string &start_barcode, const std::string &scanned_by) {
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

bool BoxDataMysqlDao::rescanned(Type type, const std::string &start_barcode) {
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

std::shared_ptr<BoxData> BoxDataMysqlDao::get(const std::string &start_or_end_barcode) {
    auto box_tables = BoxTables(*db_, order_name_, "id").where("start_barcode", start_or_end_barcode).all();
    if (box_tables.size() == 0) {
        box_tables = BoxTables(*db_, order_name_, "id").where("end_barcode", start_or_end_barcode).all();
        if (box_tables.size() == 0) {
            return nullptr;
        }
    }

    auto box_data           = std::make_shared<BoxData>();
    box_data->id            = box_tables[0]("id").asInt();
    box_data->filename      = box_tables[0]("filename").asString();
    box_data->box_number    = box_tables[0]("box_number").asString();
    box_data->start_number  = box_tables[0]("start_number").asString();
    box_data->end_number    = box_tables[0]("end_number").asString();
    box_data->quantity      = box_tables[0]("quantity").asInt();
    box_data->start_barcode = box_tables[0]("start_barcode").asString();
    box_data->end_barcode   = box_tables[0]("end_barcode").asString();
    box_data->status        = box_tables[0]("status").asInt();
    box_data->card_status   = box_tables[0]("card_status").asInt();
    box_data->carton_status = box_tables[0]("carton_status").asInt();
    box_data->scanned_by    = box_tables[0]("scanned_by").asString();
    box_data->scanned_at    = box_tables[0]("scanned_at").asString();

    return box_data;
}

bool BoxDataMysqlDao::update(const int &id, std::shared_ptr<BoxData> &box_data) {
    BoxTables(*db_, order_name_, "id")
        .where("id", id)
        .update({
            {"filename", box_data->filename},
            {"box_number", box_data->box_number},
            {"start_number", box_data->start_number},
            {"end_number", box_data->end_number},
            {"quantity", box_data->quantity},
            {"start_barcode", box_data->start_barcode},
            {"end_barcode", box_data->end_barcode},
            {"status", box_data->status},
            {"card_status", box_data->card_status},
            {"carton_status", box_data->carton_status},
            {"scanned_by", box_data->scanned_by},
            {"scanned_at", box_data->scanned_at},
        });

    return true;
}

bool BoxDataMysqlDao::clear() {
    std::string sql = "DROP TABLE  IF EXISTS `" + order_name_ + "`";
    db_->execute(sql);

    return true;
}

void BoxDataMysqlDao::init() {
    // 读取配置文件
    QSettings settings("config.ini", QSettings::IniFormat);
    settings.beginGroup("mysql");
    std::string host     = settings.value("host").toString().toStdString();
    int         port     = settings.value("port").toInt();
    std::string user     = settings.value("user").toString().toStdString();
    std::string password = settings.value("password").toString().toStdString();
    std::string database = "box_data";
    settings.endGroup();

    db_ = std::make_shared<zel::myorm::Database>();
    if (!db_->connect(host, port, user, password, database)) {
        return;
    }

    auto box_tables = BoxTables(*db_, order_name_, "id");

    if (!box_tables.exists()) {
        std::string sql = "CREATE TABLE IF NOT EXISTS `" + order_name_ +
            "` ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "filename VARCHAR(255) NOT NULL,"
            "box_number VARCHAR(255) NOT NULL,"
            "start_number VARCHAR(255) NOT NULL,"
            "end_number VARCHAR(255) NOT NULL,"
            "quantity INT NOT NULL,"
            "start_barcode VARCHAR(255) NOT NULL,"
            "end_barcode VARCHAR(255) NOT NULL,"
            "status INT NOT NULL DEFAULT 0,"
            "card_status INT NOT NULL DEFAULT 0,"
            "carton_status INT NOT NULL DEFAULT 0,"
            "scanned_by VARCHAR(255) DEFAULT NULL,"
            "scanned_at VARCHAR(255) DEFAULT NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;";

        db_->execute(sql);
    }
}