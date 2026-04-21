#pragma once

#include "database/box_data/box_data_dao_factory.h"
#include "database/card_data/card_data_dao_factory.h"
#include "database/carton_data/carton_data_dao_factory.h"
#include "database/order/order_dao.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <qobject>
#include <qthread>
#include <xlnt/workbook/workbook.hpp>
#include <xlnt/xlnt.hpp>
#include <zel/myorm.h>

class ExportExcel : public QThread {
    Q_OBJECT

  public:
    ExportExcel(const std::shared_ptr<SQLite::Database> &sqlite_db, const std::shared_ptr<zel::myorm::Database> &mysql_db, const std::string &order_name,
                OrderDao::Type type, int status, const std::string &output_file_path)
        : sqlite_db_(sqlite_db)
        , mysql_db_(mysql_db)
        , order_name_(order_name)
        , type_(type)
        , status_(status)
        , output_file_path_(output_file_path) {}

    void run() override {
        switch (type_) {
        case OrderDao::Type::BOX:
            exportBox();
            break;

        case OrderDao::Type::CARTON:
            exportCarton();
            break;

        case OrderDao::Type::CARD:
            exportCard();
            break;

        default:
            break;
        }
    }

    void exportBox() {
        auto                                  box_data_dao = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_name_);
        std::vector<std::shared_ptr<BoxData>> box_datas;

        if (status_ == -1) {
            box_datas = box_data_dao->all();
        } else {
            box_datas = box_data_dao->all(BoxDataDao::Type::BOX, status_);
        }

        if (box_datas.empty()) {
            emit exportExcelFailure();
            return;
        }

        xlnt::workbook wb;
        auto           ws = wb.active_sheet();

        // 表头
        std::vector<std::string> haders = {"序号", "盒号", "起始条码", "扫描详情", "操作人", "操作时间"};
        for (size_t i = 0; i < haders.size(); i++) {
            ws.cell(i + 1, 1).value(haders[i]);
        }

        // 数据
        int row = 2;
        int sn  = 1;
        for (const auto &box_data : box_datas) {
            ws.cell(1, row).value(sn);
            ws.cell(2, row).value(box_data->box_number);
            ws.cell(3, row).value(box_data->start_barcode);
            ws.cell(4, row).value(box_data->status == 1 ? "已扫描" : "未扫描");
            ws.cell(5, row).value(box_data->scanned_by);
            ws.cell(6, row).value(box_data->scanned_at);
            row++;
            sn++;
        }

        wb.save(output_file_path_);
        emit exportExcelSuccess();
    }

    void exportCarton() {
        auto                                     carton_data_dao = CartonDataDaoFactory::create(sqlite_db_, mysql_db_, order_name_);
        std::vector<std::shared_ptr<CartonData>> carton_datas;

        if (status_ == -1) {
            carton_datas = carton_data_dao->all();
        } else {
            carton_datas = carton_data_dao->all(status_);
        }

        if (carton_datas.empty()) {
            emit exportExcelFailure();
            return;
        }

        xlnt::workbook wb;
        auto           ws = wb.active_sheet();

        // 表头
        std::vector<std::string> haders = {"序号", "箱号", "起始条码", "扫描详情", "操作人", "操作时间"};
        for (size_t i = 0; i < haders.size(); i++) {
            ws.cell(i + 1, 1).value(haders[i]);
        }

        // 数据
        int row = 2;
        int sn  = 1;
        for (const auto &carton_data : carton_datas) {
            ws.cell(1, row).value(sn);
            ws.cell(2, row).value(carton_data->carton_number);
            ws.cell(3, row).value(carton_data->start_barcode);
            ws.cell(4, row).value(carton_data->status == 1 ? "已扫描" : "未扫描");
            ws.cell(5, row).value(carton_data->scanned_by);
            ws.cell(6, row).value(carton_data->scanned_at);
            row++;
            sn++;
        }

        wb.save(output_file_path_);
        emit exportExcelSuccess();
    }

    void exportCard() {
        auto                                   card_data_dao = CardDataDaoFactory::create(sqlite_db_, mysql_db_, order_name_);
        std::vector<std::shared_ptr<CardData>> card_datas;

        if (status_ == -1) {
            card_datas = card_data_dao->all();
        } else {
            card_datas = card_data_dao->all(status_);
        }

        if (card_datas.empty()) {
            emit exportExcelFailure();
            return;
        }

        xlnt::workbook wb;
        auto           ws = wb.active_sheet();

        // 表头
        std::vector<std::string> haders = {"序号", "卡号", "起始条码", "扫描详情", "操作人", "操作时间"};
        for (size_t i = 0; i < haders.size(); i++) {
            ws.cell(i + 1, 1).value(haders[i]);
        }

        // 数据
        int row = 2;
        int sn  = 1;
        for (const auto &card_data : card_datas) {
            ws.cell(1, row).value(sn);
            ws.cell(2, row).value(card_data->card_number);
            ws.cell(3, row).value(card_data->iccid_barcode);
            ws.cell(4, row).value(card_data->status == 1 ? "已扫描" : "未扫描");
            ws.cell(5, row).value(card_data->scanned_by);
            ws.cell(6, row).value(card_data->scanned_at);
            row++;
            sn++;
        }

        wb.save(output_file_path_);
        emit exportExcelSuccess();
    }

  signals:
    // 信号函数，用于向外界发射信号
    void exportExcelSuccess();
    void exportExcelFailure();

  private:
    std::shared_ptr<SQLite::Database>     sqlite_db_;
    std::shared_ptr<zel::myorm::Database> mysql_db_;
    std::string                           order_name_;
    OrderDao::Type                        type_;
    int                                   status_;
    std::string                           output_file_path_;
};