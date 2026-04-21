#include "main_window.h"

#include "app/component/order_group_box.hpp"
#include "comparison/comparison.h"
#include "database/box_data/box_data_dao_factory.h"
#include "database/card_data/card_data_dao_factory.h"
#include "database/carton_data/carton_data_dao_factory.h"
#include "database/format/format_dao_factory.h"
#include "database/mode/mode_dao_factory.h"
#include "database/order/order_dao_factory.h"
#include "database/role/role_dao_factory.h"
#include "login.h"
#include "setting.h"
#include "task/export_excel.hpp"
#include "task/handle_order.hpp"
#include "utils/utils.h"

#include <SQLiteCpp/Database.h>
#include <memory>
#include <qaction.h>
#include <qaction>
#include <qboxlayout.h>
#include <qchar.h>
#include <qcombobox.h>
#include <qcompleter>
#include <qdebug>
#include <qfiledialog>
#include <qkeyevent>
#include <qlayout.h>
#include <qlineedit>
#include <qmessagebox>
#include <qpushbutton.h>
#include <qpushbutton>
#include <queue>
#include <vector>
#include <zel/core.h>

using namespace zel::fs;
using namespace zel::utility;

MainWindow::MainWindow(const std::shared_ptr<SQLite::Database> &sqlite_db, const std::shared_ptr<zel::myorm::Database> &mysql_db,
                       const std::shared_ptr<UserDao> &user_dao, QMainWindow *parent)
    : QMainWindow(parent)
    , ui_(new Ui_MainWindow)
    , loading_(std::make_shared<Loading>())
    , auth_(std::make_shared<Auth>(user_dao))
    , current_box_number_(-1)
    , sqlite_db_(sqlite_db)
    , mysql_db_(mysql_db)
    , user_dao_(user_dao) {
    ui_->setupUi(this);
    init_window();

    init_dao();

    init_ui();

    init_signals_slots();

    refreshBoxTab();
}

MainWindow::~MainWindow() { delete ui_; }

void MainWindow::switchUserActionTriggered() {
    // 关闭键盘监听
    this->releaseKeyboard();

    Login *login = new Login();
    login->show();
    this->close();
}

void MainWindow::chineseActionTriggered() { switch_language("zh_CN"); }

void MainWindow::englishActionTriggered() { switch_language("en_US"); }

void MainWindow::setTagDataActionTriggered() {
    Setting *setting = new Setting();
    setting->show();
}

void MainWindow::boxSelectOrder() {
    if (ui_->box_order_name_combo->currentText().isEmpty()) {
        return;
    }

    ui_->box_check_format_label->clear();
    ui_->box_datas_status_comb_box->setEnabled(true);

    // 获取订单信息
    std::shared_ptr<Order> order = order_dao_->get(ui_->box_order_name_combo->currentText().toStdString());
    if (!order_dao_->currentOrder(order->id)) {
        return;
    }

    // 显示订单信息
    ui_->box_check_format_label->setText(QString::fromStdString(order->check_format));
    refreshBoxTable(order->name, ui_->box_datas_status_comb_box->currentIndex() - 1);

    ui_->box_start_or_end_line->setEnabled(true);
    ui_->card_start_line->setEnabled(true);
    ui_->card_end_line->setEnabled(true);

    // 聚焦到起始条码输入框
    ui_->box_start_or_end_line->setFocus();
}

void MainWindow::selectBoxDatasStatus() { refreshBoxTable(order_dao_->currentOrder()->name, ui_->box_datas_status_comb_box->currentIndex() - 1); }

void MainWindow::toCardStartBarcode() {
    ui_->card_start_line->setFocus();
    scroll_to_value(ui_->box_table, ui_->box_start_or_end_line->text());
}

void MainWindow::toCardEndBarcode() { ui_->card_end_line->setFocus(); }

void MainWindow::compareBox() {
    auto       box_data_dao = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    Comparison comparison(order_dao_->currentOrder(), box_data_dao);

    QString error, log_msg;
    auto    box_info                   = std::make_shared<BoxInfo>();
    box_info->box_start_or_end_barcode = ui_->box_start_or_end_line->text();
    box_info->card_start_barcode       = ui_->card_start_line->text();
    box_info->card_end_barcode         = ui_->card_end_line->text();

    int result = comparison.box(box_info);
    if (result == 0) {
        log_msg = QString::asprintf("用户[%s] 内盒起始或结束条码[%s] 首卡条码[%s] 尾卡条码[%s], 扫描成功", user_dao_->currentUser()->description.c_str(),
                                    box_info->box_start_or_end_barcode.toStdString().c_str(), box_info->card_start_barcode.toStdString().c_str(),
                                    box_info->card_end_barcode.toStdString().c_str());
        box_data_dao->scanned(BoxDataDao::Type::BOX, box_info->box_start_or_end_barcode.toStdString(), user_dao_->currentUser()->description);

        refreshBoxTable(order_dao_->currentOrder()->name, ui_->box_datas_status_comb_box->currentIndex() - 1);
        scroll_to_value(ui_->box_table, ui_->box_start_or_end_line->text(), false);
    } else {
        switch (result) {

        case 1: {
            error = tr("内盒起始或结束条码不在该订单范围内, 请核对!");
            break;
        }

        case 2: {
            error = tr("不正确的首卡条码, 请核对!");
            break;
        }

        case 3: {
            error = tr("不正确的尾卡条码, 请核对!");
            break;
        }
        }

        log_msg = QString::asprintf("用户[%s] 内盒起始或结束条码[%s] 首卡条码[%s] 尾卡条码[%s], 扫描失败，失败原因[%s]", user_dao_->currentUser()->name.c_str(),
                                    box_info->box_start_or_end_barcode.toStdString().c_str(), box_info->card_start_barcode.toStdString().c_str(),
                                    box_info->card_end_barcode.toStdString().c_str(), error.toStdString().c_str());

        QMessageBox::warning(this, tr("提示"), tr("比对失败: ") + error);
    }

    QString log_file = QString("log/内盒/%1.log").arg(QString::fromStdString(order_dao_->currentOrder()->name));
    if (!log(log_file, log_msg)) {
        printf("log write error\n");
    }

    ui_->box_start_or_end_line->clear();
    ui_->card_start_line->clear();
    ui_->card_end_line->clear();

    ui_->box_start_or_end_line->setFocus();
}

void MainWindow::refreshBoxTab() {
    ui_->box_table->clearContents();
    ui_->box_start_or_end_line->clear();
    ui_->card_start_line->clear();
    ui_->card_end_line->clear();
    ui_->box_order_name_combo->clear();
    ui_->box_datas_status_comb_box->setCurrentIndex(0);
    ui_->box_check_format_label->clear();
    ui_->box_table->setRowCount(0);

    ui_->box_start_or_end_line->clearFocus();
    ui_->box_order_name_combo->clearFocus();

    ui_->box_datas_status_comb_box->setEnabled(false);
    ui_->box_start_or_end_line->setEnabled(false);
    ui_->card_start_line->setEnabled(false);
    ui_->card_end_line->setEnabled(false);

    // 设置订单号下拉框内容
    for (auto &order : order_dao_->all()) {
        ui_->box_order_name_combo->addItem(QString::fromStdString(order->name));
    }

    // 下框默认不选中
    ui_->box_order_name_combo->setCurrentText("");

    // 用户管理员 及 品质
    if (user_dao_->currentUser()->role_id == 1 || user_dao_->currentUser()->role_id == 3) {
        ui_->box_order_name_combo->setEnabled(false);
        ui_->box_start_or_end_line->setEnabled(false);
        ui_->card_start_line->setEnabled(false);
        ui_->card_end_line->setEnabled(false);
    }
}

void MainWindow::refreshBoxTable(const std::string &order_name, const int &status) {
    auto                                  box_data_dao = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_name);
    std::vector<std::shared_ptr<BoxData>> box_datas;

    if (status == -1) {
        box_datas = box_data_dao->all();
    } else {
        box_datas = box_data_dao->all(BoxDataDao::Type::BOX, status);
    }

    ui_->box_table->setRowCount(box_datas.size());
    for (int i = 0; i < int(box_datas.size()); i++) {
        // 创建 item
        auto item0 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->start_barcode));
        auto item1 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->end_barcode));
        auto item2 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->start_number));
        auto item3 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->end_number));

        if (status == -1) {
            if (box_datas[i]->status == 1) {
                QBrush greenBrush(QColor(144, 238, 144)); // 淡绿色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
            } else {
                QBrush greenBrush(QColor(233, 231, 227)); // 灰色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
            }
        }

        // 放到表格里
        ui_->box_table->setItem(i, 0, item0);
        ui_->box_table->setItem(i, 1, item1);
        ui_->box_table->setItem(i, 2, item2);
        ui_->box_table->setItem(i, 3, item3);

        // 设置内容居中
        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
        item2->setTextAlignment(Qt::AlignCenter);
        item3->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::cartonSelectOrder() {
    if (ui_->carton_order_name_combo->currentText().isEmpty()) {
        return;
    }

    ui_->carton_check_format_label->clear();

    // 获取订单信息
    auto order = order_dao_->get(ui_->carton_order_name_combo->currentText().toStdString());
    if (!order_dao_->currentOrder(order->id)) {
        return;
    }

    refreshCartonTable(order->name, ui_->carton_datas_status_comb_box->currentIndex() - 1);

    // 显示订单信息
    ui_->carton_check_format_label->setText(QString::fromStdString(order->check_format));

    ui_->carton_datas_status_comb_box->setEnabled(true);
    ui_->carton_start_line->setEnabled(true);
    ui_->carton_end_line->setEnabled(true);
    ui_->target_line->setEnabled(true);

    ui_->carton_start_line->setFocus();
}

void MainWindow::showSelectedCarton() {
    // 获取当前选择模型
    QItemSelectionModel *selectModel = ui_->carton_table->selectionModel();
    if (!selectModel || ui_->carton_table->rowCount() == 0) {
        return; // 没有数据，直接返回
    }

    // 获取选中的行
    QModelIndexList indexes = selectModel->selectedRows();
    if (indexes.isEmpty()) {
        return; // 没有选中行
    }

    QStringList rowData;
    for (const QModelIndex &index : indexes) {
        int row = index.row();
        if (row < 0 || row >= ui_->carton_table->rowCount()) {
            continue; // 避免越界访问
        }

        for (int col = 0; col < ui_->carton_table->columnCount(); ++col) {
            QModelIndex idx = ui_->carton_table->model()->index(row, col);
            if (idx.isValid()) {
                rowData << ui_->carton_table->model()->data(idx).toString();
            }
        }
    }

    if (!rowData.isEmpty()) {
        refreshBoxCompareGroup(5, rowData[0].toStdString());
    }
}

void MainWindow::selectCartonDatasStatus() {
    if (ui_->box_compare_group_box->layout()) {
        clear_box_compare_group_layout(ui_->box_compare_group_box->layout());
        delete ui_->box_compare_group_box->layout();
    }
    refreshCartonTable(order_dao_->currentOrder()->name, ui_->carton_datas_status_comb_box->currentIndex() - 1);
}

void MainWindow::toCartonEndBarcode() {
    ui_->carton_start_line->setEnabled(false);
    ui_->carton_end_line->setFocus();
}

void MainWindow::toTargetBarcode() {
    ui_->carton_end_line->setEnabled(false);
    ui_->target_line->setFocus();
    scroll_to_value(ui_->carton_table, ui_->carton_start_line->text());
}

void MainWindow::compareCarton() {
    std::shared_ptr<CartonInfo> carton_info = std::make_shared<CartonInfo>();
    carton_info->carton_start_barcode       = ui_->carton_start_line->text();
    carton_info->carton_end_barcode         = ui_->carton_end_line->text();
    carton_info->target_barcode             = ui_->target_line->text();

    QString error, log_msg;
    bool    is_end          = false;
    auto    box_data_dao    = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto    carton_data_dao = CartonDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto    comparison      = std::make_shared<Comparison>(order_dao_->currentOrder(), box_data_dao, carton_data_dao);
    int     box_widget_id   = 0;
    int     result          = comparison->carton(carton_info, box_widget_id);

    // Check if the target barcodes are sequence.
    if (box_widgets_.front()->id() != box_widget_id) {
        if (result == 0) {
            result = 4;
        }
    }

    if (result == 0) {
        log_msg = QString::asprintf("用户[%s] 外箱起始[%s] 结束条码[%s] 内盒起始或结束条码[%s], 扫描成功", user_dao_->currentUser()->name.c_str(),
                                    carton_info->carton_start_barcode.toStdString().c_str(), carton_info->carton_end_barcode.toStdString().c_str(),
                                    carton_info->target_barcode.toStdString().c_str());
        box_widgets_.front()->scanned();

        box_widgets_.pop();
        if (box_widgets_.empty()) {
            is_end = true;
        } else {
            box_widgets_.front()->pending();
        }
    } else {
        switch (result) {

        case 1: {
            error = tr("外箱起始条码不在该订单范围内, 请核对!");
            break;
        }

        case 2: {
            error = tr("外箱结束条码不正确, 请核对!");
            break;
        }

        case 3: {
            error = tr("内盒条码不在该外箱范围内, 请核对!");
            break;
        }

        case 4: {
            error = tr("内盒顺序错误, 请核对!");
            break;
        }
        }

        log_msg = QString::asprintf("用户[%s] 外箱起始[%s] 结束条码[%s] 内盒起始或结束条码[%s], 扫描失败，失败原因[%s]",
                                    user_dao_->currentUser()->description.c_str(), carton_info->carton_start_barcode.toStdString().c_str(),
                                    carton_info->carton_end_barcode.toStdString().c_str(), carton_info->target_barcode.toStdString().c_str(),
                                    error.toStdString().c_str());

        QMessageBox::warning(this, tr("提示"), tr("比对失败: ") + error);
    }

    QString log_file = QString("log/外箱/%1.log").arg(QString::fromStdString(order_dao_->currentOrder()->name));
    if (!log(log_file, log_msg)) {
        printf("log write error\n");
    }

    if (is_end) {
        carton_data_dao->scanned(carton_info->carton_start_barcode.toStdString(), user_dao_->currentUser()->description);
        refreshCartonTable(order_dao_->currentOrder()->name, ui_->carton_datas_status_comb_box->currentIndex() - 1);
        scroll_to_value(ui_->carton_table, ui_->carton_start_line->text(), false);

        ui_->carton_start_line->clear();
        ui_->carton_end_line->clear();
        ui_->target_line->clear();

        ui_->carton_start_line->setEnabled(true);
        ui_->carton_end_line->setEnabled(true);

        ui_->carton_start_line->setFocus();
    } else {
        ui_->target_line->clear();
        ui_->target_line->setFocus();
    }
}

void MainWindow::refreshCartonTab() {
    ui_->carton_table->clearContents();
    ui_->carton_order_name_combo->clear();
    ui_->carton_check_format_label->clear();
    ui_->carton_start_line->clear();
    ui_->carton_end_line->clear();
    ui_->carton_table->setRowCount(0);
    ui_->carton_datas_status_comb_box->setCurrentIndex(0);

    if (ui_->box_compare_group_box->layout()) {
        clear_box_compare_group_layout(ui_->box_compare_group_box->layout());
        delete ui_->box_compare_group_box->layout();
    }

    ui_->carton_start_line->clearFocus();
    ui_->carton_end_line->clearFocus();
    ui_->carton_order_name_combo->clearFocus();

    ui_->carton_datas_status_comb_box->setEnabled(false);
    ui_->carton_start_line->setEnabled(false);
    ui_->carton_end_line->setEnabled(false);
    ui_->target_line->setEnabled(false);

    // 设置订单号下拉框内容
    for (auto &order : order_dao_->all()) {
        ui_->carton_order_name_combo->addItem(QString::fromStdString(order->name));
    }

    // 下框默认不选中
    ui_->carton_order_name_combo->setCurrentText("");

    // 用户管理员 及 品质
    if (user_dao_->currentUser()->role_id == 1 || user_dao_->currentUser()->role_id == 3) {
        ui_->carton_order_name_combo->setEnabled(false);
        ui_->carton_start_line->setEnabled(false);
        ui_->carton_end_line->setEnabled(false);
        ui_->target_line->setEnabled(false);
    }
}

void MainWindow::refreshCartonTable(const std::string &order_name, const int &status) {
    auto                                     carton_data_dao = CartonDataDaoFactory::create(sqlite_db_, mysql_db_, order_name);
    std::vector<std::shared_ptr<CartonData>> carton_datas;

    if (status == -1) {
        carton_datas = carton_data_dao->all();
    } else {
        carton_datas = carton_data_dao->all(status);
    }

    ui_->carton_table->setRowCount(carton_datas.size());

    for (int i = 0; i < int(carton_datas.size()); i++) {
        // 创建 item
        auto item0 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->start_barcode));
        auto item1 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->end_barcode));
        auto item2 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->start_number));
        if (status == -1) {
            if (carton_datas[i]->status == 1) {
                QBrush greenBrush(QColor(144, 238, 144)); // 淡绿色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
            } else {
                QBrush greenBrush(QColor(233, 231, 227)); // 灰色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
            }
        }

        // 放到表格里
        ui_->carton_table->setItem(i, 0, item0);
        ui_->carton_table->setItem(i, 1, item1);
        ui_->carton_table->setItem(i, 2, item2);

        // 设置内容居中
        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
        item2->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::refreshBoxCompareGroup(const int &cols, const std::string &selected_carton_start_barcode) {
    box_widgets_ = std::queue<BoxWidget *>();

    if (ui_->box_compare_group_box->layout()) {
        clear_box_compare_group_layout(ui_->box_compare_group_box->layout());
        delete ui_->box_compare_group_box->layout();
    }

    QGridLayout *layout = new QGridLayout(ui_->box_compare_group_box);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setAlignment(Qt::AlignCenter);

    auto box_data_dao    = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto carton_data_dao = CartonDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto carton_data     = carton_data_dao->get(selected_carton_start_barcode);

    auto box_datas = box_data_dao->all(carton_data->start_number, carton_data->end_number);
    for (int i = 0; i < int(box_datas.size()); ++i) {
        BoxWidget *box = new BoxWidget(box_datas[i]->id, QString::fromStdString(box_datas[i]->box_number));

        int row = i / cols;
        int col = i % cols;
        layout->addWidget(box, row, col, Qt::AlignCenter);

        if (i == 0) {
            box->pending();
        }

        if (carton_data->status) {
            box->scanned();
        }

        box_widgets_.push(box);
    }
}

void MainWindow::cardSelectOrder() {
    if (ui_->card_order_name_combo->currentText().isEmpty()) {
        return;
    }

    ui_->card_check_format_label->clear();

    // 获取订单信息
    auto order = order_dao_->get(ui_->card_order_name_combo->currentText().toStdString());
    if (!order_dao_->currentOrder(order->id)) {
        return;
    }

    refreshCardTable(order->name, ui_->carton_datas_status_comb_box->currentIndex() - 1);

    // 显示订单信息
    ui_->card_check_format_label->setText(QString::fromStdString(order->check_format));

    ui_->card_datas_status_comb_box->setEnabled(true);
    ui_->card_box_start_or_end_line->setEnabled(true);
    ui_->card_rescanned_btn->setEnabled(true);

    ui_->card_box_start_or_end_line->setFocus();
}

void MainWindow::cardResccenedBtnClicked() {
    std::string box_barcode = ui_->card_box_start_or_end_line->text().toStdString();
    if (box_barcode.empty()) {
        QMessageBox::warning(this, tr("提示"), tr("请输入内盒起始条码!"));
        return;
    }

    auto box_data_dao = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto box_data     = box_data_dao->get(box_barcode);
    if (!box_data) {
        QMessageBox::warning(this, tr("提示"), tr("内盒起始条码不在该订单范围内, 请核对!"));
        return;
    }
    box_data_dao->rescanned(BoxDataDao::CARD, box_barcode);

    auto card_data_dao = CardDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    card_data_dao->rescanned(box_data->start_barcode, box_data->end_barcode);

    refreshCardTable(order_dao_->currentOrder()->name, ui_->card_datas_status_comb_box->currentIndex() - 1);
    scroll_to_value(ui_->card_table, ui_->card_box_start_or_end_line->text(), true);
    refreshCardCompareGroup(20, box_barcode);

    ui_->card_box_start_or_end_line->clear();
    ui_->card_line->clear();
    ui_->card_label_line->clear();

    ui_->card_box_start_or_end_line->setEnabled(true);

    ui_->card_box_start_or_end_line->setFocus();
}

void MainWindow::showSelectedCard() {
    // 获取当前选择模型
    QItemSelectionModel *selectModel = ui_->card_table->selectionModel();
    if (!selectModel || ui_->card_table->rowCount() == 0) {
        return; // 没有数据，直接返回
    }

    // 获取选中的行
    QModelIndexList indexes = selectModel->selectedRows();
    if (indexes.isEmpty()) {
        return; // 没有选中行
    }

    QStringList rowData;
    for (const QModelIndex &index : indexes) {
        int row = index.row();
        if (row < 0 || row >= ui_->card_table->rowCount()) {
            continue; // 避免越界访问
        }

        for (int col = 0; col < ui_->card_table->columnCount(); ++col) {
            QModelIndex idx = ui_->card_table->model()->index(row, col);
            if (idx.isValid()) {
                rowData << ui_->card_table->model()->data(idx).toString();
            }
        }
    }

    if (!rowData.isEmpty()) {
        refreshCardCompareGroup(20, rowData[0].toStdString());
    }
}

void MainWindow::selectCardDatasStatus() {
    if (ui_->card_compare_group_box->layout()) {
        clear_box_compare_group_layout(ui_->card_compare_group_box->layout());
        delete ui_->card_compare_group_box->layout();
    }

    refreshCardTable(order_dao_->currentOrder()->name, ui_->card_datas_status_comb_box->currentIndex() - 1);
}

void MainWindow::toCardLabelBarcode() {
    ui_->card_label_line->setEnabled(true);
    ui_->card_line->setEnabled(false);
    ui_->card_stop_label_btn->setEnabled(true);

    ui_->card_box_start_or_end_line->setEnabled(false);
    ui_->card_label_line->setFocus();

    current_box_number_ = scroll_to_value(ui_->card_table, ui_->card_box_start_or_end_line->text());
}

void MainWindow::compareCardLabel() {
    std::shared_ptr<CardInfo> card_info = std::make_shared<CardInfo>();
    card_info->box_start_or_end_barcode = ui_->card_box_start_or_end_line->text();
    card_info->label_barcode            = ui_->card_label_line->text();

    QString error, log_msg;
    bool    is_end         = false;
    auto    box_data_dao   = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto    card_data_dao  = CardDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto    comparison     = std::make_shared<Comparison>(order_dao_->currentOrder(), box_data_dao, nullptr, card_data_dao);
    int     card_widget_id = 0;
    int     result         = comparison->card(card_info, card_widget_id);

    // Check if the card barcodes are sequence.
    if (card_label_widgets_.front()->cardData()->id != card_widget_id) {
        if (result == 0) {
            result = 3;
        }
    }

    if (result == 0) {
        log_msg = QString::asprintf("用户[%s] 内盒起始条码[%s] 标签条码[%s], 扫描成功", user_dao_->currentUser()->name.c_str(),
                                    card_info->box_start_or_end_barcode.toStdString().c_str(), card_info->label_barcode.toStdString().c_str());
        card_label_widgets_.front()->pending();

        card_label_widgets_.pop();
        if (card_label_widgets_.empty()) {
            is_end = true;
        }

        card_label_barcodes_.push(card_info->label_barcode);
    } else {
        switch (result) {

        case 1: {
            error = tr("内盒起始条码不在该订单范围内, 请核对!");
            break;
        }

        case 2: {
            error = tr("标签条码不在该内盒范围内, 请核对!");
            break;
        }

        case 3: {
            error = tr("标签顺序错误, 请核对!");
            break;
        }
        }

        log_msg = QString::asprintf("用户[%s] 内盒起始条码[%s] 标签条码[%s], 扫描失败，失败原因[%s]", user_dao_->currentUser()->name.c_str(),
                                    card_info->box_start_or_end_barcode.toStdString().c_str(), card_info->label_barcode.toStdString().c_str(),
                                    error.toStdString().c_str());

        QMessageBox::warning(this, tr("提示"), tr("比对失败: ") + error);
    }

    QString log_file =
        QString("log/卡片/%1/内盒%2.log").arg(QString::fromStdString(order_dao_->currentOrder()->name)).arg(current_box_number_, 4, 10, QLatin1Char('0'));
    if (!log(log_file, log_msg)) {
        printf("log write error\n");
    }

    if (result == 0) {
        toCardBarcode();
    } else {
        ui_->card_label_line->clear();
        toCardLabelBarcode();
    }
}

void MainWindow::toCardBarcode() {
    ui_->card_line->setEnabled(true);
    // ui_->card_label_line->clear();
    ui_->card_label_line->setEnabled(false);
    ui_->card_line->setFocus();
}

void MainWindow::compareCard() {
    if (card_label_barcodes_.empty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先扫描标签条码!"));
        return;
    }

    std::shared_ptr<CardInfo> card_info = std::make_shared<CardInfo>();
    card_info->box_start_or_end_barcode = ui_->card_box_start_or_end_line->text();
    card_info->label_barcode            = card_label_barcodes_.front();
    card_info->card_barcode             = ui_->card_line->text();

    QString error, log_msg;
    auto    box_data_dao  = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto    card_data_dao = CardDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);

    auto end_number = box_data_dao->get(card_info->box_start_or_end_barcode.toStdString())->end_number;
    bool is_box_end = false;

    bool is_success = card_info->label_barcode == card_info->card_barcode;
    if (card_info->card_barcode.toStdString() == end_number) {
        is_box_end = true;
    }

    bool is_batch_end = false;
    if (is_success) {
        log_msg = QString::asprintf("用户[%s] 内盒起始条码[%s] 标签条码[%s] 卡片条码[%s], 扫描成功", user_dao_->currentUser()->description.c_str(),
                                    card_info->box_start_or_end_barcode.toStdString().c_str(), card_info->label_barcode.toStdString().c_str(),
                                    card_info->card_barcode.toStdString().c_str());
        card_data_dao->scanned(card_info->card_barcode.toStdString(), user_dao_->currentUser()->description);
        card_widgets_.front()->scanned();

        card_widgets_.pop();
        card_label_barcodes_.pop();
        if (card_widgets_.empty()) {
            is_batch_end = true;
        }
    } else {
        error = tr("卡片条码与标签条码不匹配, 请核对!");

        log_msg = QString::asprintf("用户[%s] 内盒起始条码[%s] 卡片条码[%s] 标签条码[%s], 扫描失败，失败原因[%s]",
                                    user_dao_->currentUser()->description.c_str(), card_info->box_start_or_end_barcode.toUtf8().constData(),
                                    card_info->card_barcode.toUtf8().constData(), card_info->label_barcode.toUtf8().constData(), error.toUtf8().constData()

        );

        QMessageBox::warning(this, tr("提示"), tr("比对失败: ") + error);
    }

    QString log_file =
        QString("log/卡片/%1/内盒%2.log").arg(QString::fromStdString(order_dao_->currentOrder()->name)).arg(current_box_number_, 4, 10, QLatin1Char('0'));
    if (!log(log_file, log_msg)) {
        printf("log write error\n");
    }

    if (is_box_end) {
        if (is_batch_end)
            box_data_dao->scanned(BoxDataDao::Type::CARD, card_info->box_start_or_end_barcode.toStdString(), user_dao_->currentUser()->description);

        refreshCardTable(order_dao_->currentOrder()->name, ui_->card_datas_status_comb_box->currentIndex() - 1);
        scroll_to_value(ui_->card_table, ui_->card_box_start_or_end_line->text(), false);

        ui_->card_box_start_or_end_line->clear();
        ui_->card_label_line->clear();
        ui_->card_line->clear();

        ui_->card_box_start_or_end_line->setEnabled(true);
        ui_->card_label_line->setEnabled(false);
        ui_->card_line->setEnabled(false);

        ui_->card_box_start_or_end_line->setFocus();
    } else {
        if (is_success) {
            ui_->card_label_line->clear();
            ui_->card_line->clear();

            ui_->card_label_line->setEnabled(true);
            ui_->card_line->setEnabled(false);

            ui_->card_label_line->setFocus();
        } else {
            ui_->card_line->clear();
            ui_->card_line->setFocus();
        }
    }
}

void MainWindow::refreshCardTab() {
    card_label_barcodes_ = std::queue<QString>();
    ui_->card_table->clearContents();
    ui_->card_order_name_combo->clear();
    ui_->card_check_format_label->clear();
    ui_->card_box_start_or_end_line->clear();
    ui_->card_label_line->clear();
    ui_->card_line->clear();
    ui_->card_table->setRowCount(0);
    ui_->card_datas_status_comb_box->setCurrentIndex(0);

    if (ui_->card_compare_group_box->layout()) {
        clear_box_compare_group_layout(ui_->card_compare_group_box->layout());
        delete ui_->card_compare_group_box->layout();
    }

    ui_->card_box_start_or_end_line->clearFocus();
    ui_->card_order_name_combo->clearFocus();

    ui_->card_datas_status_comb_box->setEnabled(false);
    ui_->card_box_start_or_end_line->setEnabled(false);
    ui_->card_line->setEnabled(false);
    ui_->card_label_line->setEnabled(false);
    ui_->card_rescanned_btn->setEnabled(false);
    ui_->card_stop_label_btn->setEnabled(false);

    // 设置订单号下拉框内容
    for (auto &order : order_dao_->all()) {
        ui_->card_order_name_combo->addItem(QString::fromStdString(order->name));
    }

    // 下框默认不选中
    ui_->card_order_name_combo->setCurrentText("");

    // 用户管理员 及 品质
    if (user_dao_->currentUser()->role_id == 1 || user_dao_->currentUser()->role_id == 3) {
        ui_->card_order_name_combo->setEnabled(false);
        ui_->card_box_start_or_end_line->setEnabled(false);
        ui_->card_line->setEnabled(false);
        ui_->card_label_line->setEnabled(false);
    }
}

void MainWindow::refreshCardTable(const std::string &order_name, const int &status) {
    auto                                  box_data_dao = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_name);
    std::vector<std::shared_ptr<BoxData>> box_datas;

    if (status == -1) {
        box_datas = box_data_dao->all();
    } else {
        box_datas = box_data_dao->all(BoxDataDao::Type::CARD, status);
    }

    ui_->card_table->setRowCount(box_datas.size());
    for (int i = 0; i < int(box_datas.size()); i++) {
        // 创建 item
        auto item0 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->start_barcode));
        auto item1 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->end_barcode));

        if (status == -1) {
            if (box_datas[i]->card_status == 1) {
                QBrush greenBrush(QColor(144, 238, 144)); // 淡绿色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
            } else {
                QBrush greenBrush(QColor(233, 231, 227)); // 灰色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
            }
        }

        // 放到表格里
        ui_->card_table->setItem(i, 0, item0);
        ui_->card_table->setItem(i, 1, item1);

        // 设置内容居中
        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::refreshCardCompareGroup(const int &cols, const std::string &selected_box_start_barcode) {
    card_widgets_ = std::queue<CardWidget *>();

    if (ui_->card_compare_group_box->layout()) {
        clear_box_compare_group_layout(ui_->card_compare_group_box->layout());
        delete ui_->card_compare_group_box->layout();
    }

    QGridLayout *layout = new QGridLayout(ui_->card_compare_group_box);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setAlignment(Qt::AlignCenter);

    auto card_data_dao = CardDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto box_data_dao  = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_dao_->currentOrder()->name);
    auto box_data      = box_data_dao->get(selected_box_start_barcode);

    auto card_datas = card_data_dao->all(box_data->start_number, box_data->end_number);
    for (int i = 0; i < int(card_datas.size()); ++i) {
        CardWidget *card = new CardWidget(card_datas[i]);

        int row = i / cols;
        int col = i % cols;
        layout->addWidget(card, row, col, Qt::AlignCenter);

        if (card_datas[i]->status) {
            card->scanned();
        } else {
            card_widgets_.push(card);
        }
    }

    // if (!card_widgets_.empty()) {
    //     card_widgets_.front()->pending();
    // }

    card_label_widgets_ = card_widgets_;
}

void MainWindow::selectBoxFileBtnClicked() {
    QString file_path = QFileDialog::getOpenFileName(this, tr("选择内盒标签文件路径"), "templates", tr("Excel/CSV 文件 (*.xlsx *.csv)"));

    if (file_path.isEmpty()) return;

    ui_->box_file_path_line->setText(file_path);
}

void MainWindow::selectCartonFileBtnClicked() {
    QString file_path = QFileDialog::getOpenFileName(this, tr("选择外箱标签文件路径"), "templates", tr("Excel/CSV 文件 (*.xlsx *.csv)"));

    if (file_path.isEmpty()) return;

    ui_->carton_file_path_line->setText(file_path);
}

void MainWindow::selectCardFileBtnClicked() {
    QString file_path = QFileDialog::getOpenFileName(this, tr("选择单卡标签文件路径"), "templates", tr("Excel/CSV 文件 (*.xlsx *.csv)"));

    if (file_path.isEmpty()) return;

    ui_->card_file_path_line->setText(file_path);
}

void MainWindow::addOrderBtnClicked() {
    // 获取订单信息
    std::string order_name             = ui_->order_name_line->text().toStdString();
    int         box_start_check_num    = ui_->box_start_spin_box->value();
    int         box_end_check_num      = ui_->box_end_spin_box->value();
    int         carton_start_check_num = ui_->carton_start_spin_box->value();
    int         carton_end_check_num   = ui_->carton_end_spin_box->value();
    int         card_start_check_num   = ui_->card_start_spin_box->value();
    int         card_end_check_num     = ui_->card_end_spin_box->value();
    int         mode_id                = 1;
    std::string box_file_path          = ui_->box_file_path_line->text().toStdString();
    std::string carton_file_path       = ui_->carton_file_path_line->text().toStdString();
    std::string card_file_path         = ui_->card_file_path_line->text().toStdString();
    int         box_scanned_num        = 0;
    int         carton_scanned_num     = 0;

    QString check_format;
    check_format = QString::asprintf("卡片：%d 位 - %d 位\n内盒：%d 位 - %d 位\n外箱：%d 位 - %d 位", card_start_check_num, card_end_check_num,
                                     box_start_check_num, box_end_check_num, carton_start_check_num, carton_end_check_num);

    std::string create_at = utils::Utils::now();

    if (order_name == "" || box_end_check_num == 0) {
        QMessageBox::warning(this, tr("添加失败"), tr("订单号或校验位数不能为空"));
        return;
    }

    if (mode_id == 0) {
        QMessageBox::warning(this, tr("添加失败"), tr("请选择条码模式"));
        return;
    }

    if (box_file_path == "" || carton_file_path == "") {
        QMessageBox::warning(this, tr("添加失败"), tr("标签文件路径不能为空"));
        return;
    }

    Order new_order{0,
                    order_name,
                    check_format.toStdString(),
                    carton_start_check_num,
                    carton_end_check_num,
                    box_start_check_num,
                    box_end_check_num,
                    card_start_check_num,
                    card_end_check_num,
                    box_scanned_num,
                    carton_scanned_num,
                    mode_id,
                    box_file_path,
                    carton_file_path,
                    card_file_path,
                    create_at};

    if (order_dao_->exists(new_order.name)) {
        QMessageBox::warning(this, tr("添加失败"), tr("订单号已存在"));
        return;
    }

    loading_->show();

    // 添加订单
    HandleOrder *handle_order = new HandleOrder(order_dao_, std::make_shared<Order>(new_order));

    connect(handle_order, &HandleOrder::addOrderSuccess, this, &MainWindow::addOrderSuccess);
    connect(handle_order, &HandleOrder::addOrderFailure, this, &MainWindow::addOrderFailure);

    handle_order->start();
}

void MainWindow::updateOrderBtnClicked() {
    // 获取选中的行索引
    int index = ui_->order_table->currentRow();

    // 获取订单信息
    std::string order_name             = ui_->order_name_line->text().toStdString();
    int         carton_start_check_num = ui_->carton_start_spin_box->value();
    int         carton_end_check_num   = ui_->carton_end_spin_box->value();
    int         box_start_check_num    = ui_->box_start_spin_box->value();
    int         box_end_check_num      = ui_->box_end_spin_box->value();
    int         card_start_check_num   = ui_->card_start_spin_box->value();
    int         card_end_check_num     = ui_->card_end_spin_box->value();
    int         mode_id                = 1;
    int         box_scanned_num        = 0;
    int         carton_scanned_num     = 0;

    QString check_format;
    check_format = QString::asprintf("卡片：%d 位 - %d 位\n内盒：%d 位 - %d 位\n外箱：%d 位 - %d 位", card_start_check_num, card_end_check_num,
                                     box_start_check_num, box_end_check_num, carton_start_check_num, carton_end_check_num);
    std::string            create_at = utils::Utils::now();
    std::shared_ptr<Order> new_order = std::make_shared<Order>(Order{0, order_name, check_format.toStdString(), carton_start_check_num, carton_end_check_num,
                                                                     box_start_check_num, box_end_check_num, card_start_check_num, card_end_check_num,
                                                                     box_scanned_num, carton_scanned_num, mode_id, "", "", "", create_at});

    if (order_name == "" || box_end_check_num == 0 || index == -1) {
        QMessageBox::warning(this, tr("修改失败"), tr("订单号或校验位数不能为空"));
        return;
    }

    // 保存订单信息
    if (order_dao_->update(order_dao_->all()[index]->id, new_order)) {
        QMessageBox::information(this, tr("保存成功"), tr("订单信息保存成功"));
        refreshOrderTab();
    } else {
        QMessageBox::warning(this, tr("保存失败"), tr("订单信息保存失败"));
    }
}

void MainWindow::removeOrderBtnClicked() {

    // 获取选中的行索引
    int index = ui_->order_table->currentRow();

    if (index == -1) {
        QMessageBox::warning(this, tr("删除失败"), tr("请选中要删除的订单"));
        return;
    }

    // 弹窗确认
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("删除订单"), tr("确认删除 [%1] 吗？").arg(QString::fromStdString(order_dao_->all()[index]->name)),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        refreshOrderTab();
        return;
    }

    // 删除订单
    if (order_dao_->remove(order_dao_->all()[index]->id)) {
        QMessageBox::information(this, tr("删除成功"), tr("订单删除成功"));
        refreshOrderTab();
    } else {
        QMessageBox::warning(this, tr("删除失败"), tr("订单删除失败"));
    }
}

void MainWindow::clearOrderBtnClicked() {
    // 弹窗确认
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("清空订单"), tr("确认清空所有订单吗？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        refreshOrderTab();
        return;
    }

    if (order_dao_->clear()) {
        QMessageBox::information(this, tr("清空成功"), tr("订单清空成功"));
        refreshOrderTab();
    } else {
        QMessageBox::warning(this, tr("清空失败"), tr("订单清空失败"));
    }
}

void MainWindow::showSelectedOrder() {
    // 获取选中的行索引
    int index = ui_->order_table->currentRow();

    ui_->order_name_line->setText(QString::fromStdString(order_dao_->all()[index]->name));
    ui_->card_start_spin_box->setValue(order_dao_->all()[index]->card_start_check_num);
    ui_->card_end_spin_box->setValue(order_dao_->all()[index]->card_end_check_num);
    ui_->box_start_spin_box->setValue(order_dao_->all()[index]->box_start_check_num);
    ui_->box_end_spin_box->setValue(order_dao_->all()[index]->box_end_check_num);
    ui_->carton_start_spin_box->setValue(order_dao_->all()[index]->carton_start_check_num);
    ui_->carton_end_spin_box->setValue(order_dao_->all()[index]->carton_end_check_num);
}

void MainWindow::refreshOrderTab() {
    auto orders = order_dao_->all();
    ui_->order_table->setRowCount(orders.size());

    // 设置表格内容
    for (int i = 0; i < int(orders.size()); i++) {
        ui_->order_table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(orders[i]->name)));
        ui_->order_table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(orders[i]->check_format)));
        ui_->order_table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(mode_dao_->get(orders[i]->mode_id)->description)));
        ui_->order_table->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(orders[i]->create_at)));

        // 设置内容居中
        ui_->order_table->item(i, 0)->setTextAlignment(Qt::AlignCenter);
        ui_->order_table->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        ui_->order_table->item(i, 2)->setTextAlignment(Qt::AlignCenter);
        ui_->order_table->item(i, 3)->setTextAlignment(Qt::AlignCenter);
    }

    // 根据内容调整列高
    ui_->order_table->resizeRowsToContents();

    ui_->order_name_line->clear();
    ui_->card_start_spin_box->setValue(1);
    ui_->card_end_spin_box->setValue(19);
    ui_->box_start_spin_box->setValue(1);
    ui_->box_end_spin_box->setValue(19);
    ui_->carton_start_spin_box->setValue(1);
    ui_->carton_end_spin_box->setValue(19);
    ui_->box_file_path_line->clear();
    ui_->carton_file_path_line->clear();
    ui_->card_file_path_line->clear();

    // 用户管理员
    if (user_dao_->currentUser()->role_id == 1) {
        ui_->order_name_line->setEnabled(false);
        ui_->box_start_spin_box->setEnabled(false);
        ui_->box_end_spin_box->setEnabled(false);
        ui_->card_start_spin_box->setEnabled(false);
        ui_->card_end_spin_box->setEnabled(false);
        ui_->carton_start_spin_box->setEnabled(false);
        ui_->carton_end_spin_box->setEnabled(false);
        ui_->box_file_path_line->setEnabled(false);
        ui_->carton_file_path_line->setEnabled(false);
        ui_->card_file_path_line->setEnabled(false);
        ui_->select_box_file_ptn->setEnabled(false);
        ui_->select_carton_file_ptn->setEnabled(false);
        ui_->select_card_file_ptn->setEnabled(false);

        ui_->add_order_btn->setEnabled(false);
        ui_->remove_order_btn->setEnabled(false);
        ui_->update_order_btn->setEnabled(false);
        ui_->clear_order_btn->setEnabled(false);
    }
}

void MainWindow::addUserBtnClicked() {
    // 获取用户信息
    std::string name        = ui_->selected_user_edit->text().toStdString();
    std::string description = ui_->selected_description_edit->text().toStdString();
    std::string password    = ui_->selected_password_edit->text().toStdString();
    int         role        = ui_->selected_combo_box->currentIndex();
    std::string language    = "zh_US";
    if (name == "" || password == "") {
        QMessageBox::warning(this, tr("添加失败"), tr("用户名或密码不能为空"));
        return;
    }

    if (role == -1) {
        QMessageBox::warning(this, tr("添加失败"), tr("请选择角色"));
        return;
    }

    User new_user = {0, name, description, password, role + 1};

    if (user_dao_->exists(new_user.name)) {
        QMessageBox::warning(this, tr("添加失败"), tr("用户已存在"));
        return;
    }

    // 添加用户
    if (user_dao_->add(std::make_shared<User>(new_user))) {
        QMessageBox::information(this, tr("添加成功"), tr("用户添加成功"));
        refreshUserTab();
    } else {
        QMessageBox::warning(this, tr("添加失败"), tr("用户添加失败"));
    }
}

void MainWindow::updateUserBtnClicked() {
    // 获取选中的行索引
    int index = ui_->user_table->currentRow();

    // 获取用户信息
    std::string name        = ui_->selected_user_edit->text().toStdString();
    std::string description = ui_->selected_description_edit->text().toStdString();
    std::string password    = ui_->selected_password_edit->text().toStdString();
    int         role        = ui_->selected_combo_box->currentIndex();
    std::string language    = "zh_US";
    User        new_user    = {0, name, description, password, role + 1};

    if (name == "" || password == "" || index == -1) {
        QMessageBox::warning(this, tr("修改失败"), tr("用户名或密码不能为空"));
        return;
    }

    // 保存用户信息
    if (user_dao_->update(user_dao_->all()[index]->id, std::make_shared<User>(new_user))) {
        QMessageBox::information(this, tr("保存成功"), tr("用户信息保存成功"));
        refreshUserTab();
    } else {
        QMessageBox::warning(this, tr("保存失败"), tr("用户信息保存失败"));
    }
}

void MainWindow::removeUserBtnClicked() {

    // 获取选中的行索引
    int index = ui_->user_table->currentRow();

    if (index == -1) {
        QMessageBox::warning(this, tr("删除失败"), tr("请选中要删除的用户"));
        return;
    }

    // 弹窗确认
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("删除用户"), tr("确认删除 [%1] 吗?").arg(QString::fromStdString(user_dao_->all()[index]->name)),
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        refreshUserTab();
        return;
    }

    // 删除用户
    if (user_dao_->remove(user_dao_->all()[index]->id)) {
        QMessageBox::information(this, tr("删除成功"), tr("用户删除成功"));
        refreshUserTab();
    } else {
        QMessageBox::warning(this, tr("删除失败"), tr("用户删除失败"));
    }
}

void MainWindow::clearUserBtnClicked() {
    // 弹窗确认
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("清空用户"), tr("确认清空所有用户吗？"), QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        refreshUserTab();
        return;
    }

    if (user_dao_->clear()) {
        QMessageBox::information(this, tr("清空成功"), tr("用户清空成功"));
        refreshUserTab();
    } else {
        QMessageBox::warning(this, tr("清空失败"), tr("用户清空失败"));
    }
}

void MainWindow::showSelectedUser() {
    // 获取选中的行索引
    int index = ui_->user_table->currentRow();

    ui_->selected_user_edit->setText(QString::fromStdString(user_dao_->all()[index]->name));
    ui_->selected_description_edit->setText(QString::fromStdString(user_dao_->all()[index]->description));
    ui_->selected_password_edit->setText(QString::fromStdString(user_dao_->all()[index]->password));
    ui_->selected_combo_box->setCurrentIndex((int) user_dao_->all()[index]->role_id - 1);
}

void MainWindow::refreshUserTab() {
    auto users = user_dao_->all();
    ui_->user_table->setRowCount(users.size());

    // 设置表格内容
    for (int i = 0; i < int(users.size()); i++) {
        ui_->user_table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(users[i]->name)));
        ui_->user_table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(users[i]->description)));
        ui_->user_table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(role_dao_->get(users[i]->role_id)->description)));

        // 设置内容居中
        ui_->user_table->item(i, 0)->setTextAlignment(Qt::AlignCenter);
        ui_->user_table->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        ui_->user_table->item(i, 2)->setTextAlignment(Qt::AlignCenter);
    }

    ui_->selected_user_edit->clear();
    ui_->selected_description_edit->clear();
    ui_->selected_password_edit->clear();
    ui_->selected_combo_box->setCurrentIndex(-1);

    // 生产 及 品质
    if (user_dao_->currentUser()->role_id == 2 || user_dao_->currentUser()->role_id == 3) {
        ui_->add_user_btn->setEnabled(false);
        ui_->remove_user_btn->setEnabled(false);
        ui_->update_user_btn->setEnabled(false);
        ui_->clear_user_btn->setEnabled(false);

        ui_->selected_user_edit->setEnabled(false);
        ui_->selected_password_edit->setEnabled(false);
        ui_->selected_description_edit->setEnabled(false);
        ui_->selected_combo_box->setEnabled(false);
    }
}

void MainWindow::searchLogBtnClicked() {
    // 获取搜索条件
    auto order_name = ui_->log_order_name_combo->currentText().toStdString();
    auto type       = ui_->log_scan_type_combo->currentIndex();
    auto status     = ui_->log_status_combo->currentIndex();

    if (order_name == "" || type == -1 || status == -1) {
        QMessageBox::warning(this, tr("搜索失败"), tr("请选择搜索条件"));
        return;
    }

    switch (status) {
    case 0:
        status = -1;
        break;

    case 1:
        status = 1;
        break;

    case 2:
        status = 0;
        break;
    }

    refreshLogTable(order_name, type, status);
}

void MainWindow::qcConfirmBtnClicked() {
    auto order_name = ui_->log_order_name_combo->currentText().toStdString();
    auto type       = ui_->log_scan_type_combo->currentIndex();
    auto status     = ui_->log_status_combo->currentIndex();

    if (order_name == "" || type == -1 || status == -1) {
        QMessageBox::warning(this, tr("确认失败"), tr("请先查询订单日志, 后进行确认!"));
        return;
    }

    auto confirm_by = ui_->log_qc_confirm_by_label->text().toStdString();
    auto confirm_at = ui_->log_qc_confirm_at_label->text().toStdString();
    if (confirm_by != "" || confirm_at != "") {
        QMessageBox::warning(this, tr("确认失败"), tr("订单已进行品质确认， 请勿重复确认!"));
        return;
    }

    auth_->show();
}

void MainWindow::exportExacelBtnClicked() {
    auto order_name = ui_->log_order_name_combo->currentText().toStdString();
    auto type_idx   = ui_->log_scan_type_combo->currentIndex();
    auto status     = ui_->log_status_combo->currentIndex();

    if (order_name == "" || type_idx == -1 || status == -1) {
        QMessageBox::warning(this, tr("导出失败"), tr("请先查询订单日志, 后进行导出!"));
        return;
    }

    QString file_dir = QFileDialog::getExistingDirectory(this, tr("选择导出 Excel 文件路径"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (file_dir.isEmpty()) {
        QMessageBox::warning(this, tr("导出失败"), tr("请选择导出 Excel 文件路径"));
        return;
    }

    loading_->show();

    OrderDao::Type type;
    switch (type_idx) {
    case 0:
        type = OrderDao::Type::BOX;
        break;
    case 1:
        type = OrderDao::Type::CARTON;
        break;
    case 2:
        type = OrderDao::Type::CARD;
        break;
    }

    switch (status) {
    case 0:
        status = -1;
        break;

    case 1:
        status = 1;
        break;

    case 2:
        status = 0;
        break;
    }

    QString file_name = order_name.c_str();
    file_name += "_";
    file_name += ui_->log_scan_type_combo->currentText();
    file_name += "_";
    file_name += ui_->log_status_combo->currentText();
    file_name += ".xlsx";
    QString file_path = QDir(file_dir).filePath(file_name);

    // 添加订单
    ExportExcel *export_excel = new ExportExcel(sqlite_db_, mysql_db_, order_name, type, status, file_path.toUtf8().constData());

    connect(export_excel, &ExportExcel::exportExcelSuccess, this, &MainWindow::exportExcelSuccess);
    connect(export_excel, &ExportExcel::exportExcelFailure, this, &MainWindow::exportExcelFailure);

    export_excel->start();
}

void MainWindow::refreshLogTab() {
    ui_->log_order_name_combo->clear();
    ui_->log_scan_type_combo->setCurrentIndex(-1);
    ui_->log_status_combo->setCurrentIndex(-1);
    ui_->log_quantity_label->setText("0");
    ui_->log_sanned_label->setText("0");
    ui_->log_unsanned_label->setText("0");
    ui_->log_qc_confirm_by_label->setText("");
    ui_->log_qc_confirm_at_label->setText("");
    ui_->log_table->setRowCount(0);

    // 设置订单号下拉框内容
    for (auto &order : order_dao_->all()) {
        ui_->log_order_name_combo->addItem(QString::fromStdString(order->name));
    }

    // 用户管理员
    if (user_dao_->currentUser()->role_id == 1) {
        ui_->log_order_name_combo->setEnabled(false);
        ui_->log_scan_type_combo->setEnabled(false);
        ui_->log_status_combo->setEnabled(false);
        ui_->log_search_btn->setEnabled(false);
        ui_->log_export_excel_btn->setEnabled(false);
        ui_->log_qc_confirm_btn->setEnabled(false);
    }
}

void MainWindow::refreshLogTable(const std::string &order_name, const int &type, const int &status) {
    switch (type) {
    // 内盒
    case 0: {
        return refreshBoxLogTable(order_name, status);
    }

    // 外箱
    case 1: {
        return refreshCartonLogTable(order_name, status);
    }

    // 卡片
    case 2: {
        return refreshCardLogTable(order_name, status);
    }

    default:
        QMessageBox::warning(this, tr("搜索失败"), tr("暂不支持该类型搜索"));
        break;
    }
}

void MainWindow::refreshBoxLogTable(const std::string &order_name, const int &status) {
    auto                                  box_data_dao = BoxDataDaoFactory::create(sqlite_db_, mysql_db_, order_name);
    std::vector<std::shared_ptr<BoxData>> box_datas;

    if (status == -1) {
        box_datas = box_data_dao->all();
    } else {
        box_datas = box_data_dao->all(BoxDataDao::Type::BOX, status);
    }

    ui_->log_quantity_label->setText(QString::number(box_data_dao->all(BoxDataDao::Type::BOX).size()));
    ui_->log_sanned_label->setText(QString::number(box_data_dao->all(BoxDataDao::Type::BOX, 1).size()));
    ui_->log_unsanned_label->setText(QString::number(box_data_dao->all(BoxDataDao::Type::BOX, 0).size()));
    ui_->log_qc_confirm_by_label->setText(QString::fromStdString(order_dao_->get(order_name)->box_confirm_by));
    ui_->log_qc_confirm_at_label->setText(QString::fromStdString(order_dao_->get(order_name)->box_confirm_at));

    ui_->log_table->setRowCount(box_datas.size());
    for (int i = 0; i < int(box_datas.size()); i++) {
        // 创建 item
        auto item0 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->box_number));
        auto item1 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->start_number));
        auto item2 = new QTableWidgetItem(box_datas[i]->status == 1 ? "已扫描" : "未扫描");
        auto item3 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->scanned_by));
        auto item4 = new QTableWidgetItem(QString::fromStdString(box_datas[i]->scanned_at));

        if (status == -1) {
            if (box_datas[i]->status == 1) {
                QBrush greenBrush(QColor(144, 238, 144)); // 淡绿色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
                item4->setBackground(greenBrush);
            } else {
                QBrush greenBrush(QColor(233, 231, 227)); // 灰色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
                item4->setBackground(greenBrush);
            }
        }

        // 放到表格里
        ui_->log_table->setItem(i, 0, item0);
        ui_->log_table->setItem(i, 1, item1);
        ui_->log_table->setItem(i, 2, item2);
        ui_->log_table->setItem(i, 3, item3);
        ui_->log_table->setItem(i, 4, item4);

        // 设置内容居中
        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
        item2->setTextAlignment(Qt::AlignCenter);
        item3->setTextAlignment(Qt::AlignCenter);
        item4->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::refreshCartonLogTable(const std::string &order_name, const int &status) {
    auto                                     carton_data_dao = CartonDataDaoFactory::create(sqlite_db_, mysql_db_, order_name);
    std::vector<std::shared_ptr<CartonData>> carton_datas;

    if (status == -1) {
        carton_datas = carton_data_dao->all();
    } else {
        carton_datas = carton_data_dao->all(status);
    }

    ui_->log_quantity_label->setText(QString::number(carton_data_dao->all().size()));
    ui_->log_sanned_label->setText(QString::number(carton_data_dao->all(1).size()));
    ui_->log_unsanned_label->setText(QString::number(carton_data_dao->all(0).size()));
    ui_->log_qc_confirm_by_label->setText(QString::fromStdString(order_dao_->get(order_name)->carton_confirm_by));
    ui_->log_qc_confirm_at_label->setText(QString::fromStdString(order_dao_->get(order_name)->carton_confirm_at));

    ui_->log_table->setRowCount(carton_datas.size());
    for (int i = 0; i < int(carton_datas.size()); i++) {
        // 创建 item
        auto item0 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->carton_number));
        auto item1 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->start_number));
        auto item2 = new QTableWidgetItem(carton_datas[i]->status == 1 ? "已扫描" : "未扫描");
        auto item3 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->scanned_by));
        auto item4 = new QTableWidgetItem(QString::fromStdString(carton_datas[i]->scanned_at));

        if (status == -1) {
            if (carton_datas[i]->status == 1) {
                QBrush greenBrush(QColor(144, 238, 144)); // 淡绿色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
                item4->setBackground(greenBrush);
            } else {
                QBrush greenBrush(QColor(233, 231, 227)); // 灰色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
                item4->setBackground(greenBrush);
            }
        }

        // 放到表格里
        ui_->log_table->setItem(i, 0, item0);
        ui_->log_table->setItem(i, 1, item1);
        ui_->log_table->setItem(i, 2, item2);
        ui_->log_table->setItem(i, 3, item3);
        ui_->log_table->setItem(i, 4, item4);

        // 设置内容居中
        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
        item2->setTextAlignment(Qt::AlignCenter);
        item3->setTextAlignment(Qt::AlignCenter);
        item4->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::refreshCardLogTable(const std::string &order_name, const int &status) {
    auto                                   card_data_dao = CardDataDaoFactory::create(sqlite_db_, mysql_db_, order_name);
    std::vector<std::shared_ptr<CardData>> card_datas;

    if (status == -1) {
        card_datas = card_data_dao->all();
    } else {
        card_datas = card_data_dao->all(status);
    }

    ui_->log_quantity_label->setText(QString::number(card_data_dao->all().size()));
    ui_->log_sanned_label->setText(QString::number(card_data_dao->all(1).size()));
    ui_->log_unsanned_label->setText(QString::number(card_data_dao->all(0).size()));
    ui_->log_qc_confirm_by_label->setText(QString::fromStdString(order_dao_->get(order_name)->card_confirm_by));
    ui_->log_qc_confirm_at_label->setText(QString::fromStdString(order_dao_->get(order_name)->card_confirm_at));

    ui_->log_table->setRowCount(card_datas.size());
    for (int i = 0; i < int(card_datas.size()); i++) {
        // 创建 item
        auto item0 = new QTableWidgetItem(QString::fromStdString(card_datas[i]->card_number));
        auto item1 = new QTableWidgetItem(QString::fromStdString(card_datas[i]->iccid_barcode));
        auto item2 = new QTableWidgetItem(card_datas[i]->status == 1 ? "已扫描" : "未扫描");
        auto item3 = new QTableWidgetItem(QString::fromStdString(card_datas[i]->scanned_by));
        auto item4 = new QTableWidgetItem(QString::fromStdString(card_datas[i]->scanned_at));

        if (status == -1) {
            if (card_datas[i]->status == 1) {
                QBrush greenBrush(QColor(144, 238, 144)); // 淡绿色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
                item4->setBackground(greenBrush);
            } else {
                QBrush greenBrush(QColor(233, 231, 227)); // 灰色
                item0->setBackground(greenBrush);
                item1->setBackground(greenBrush);
                item2->setBackground(greenBrush);
                item3->setBackground(greenBrush);
                item4->setBackground(greenBrush);
            }
        }

        // 放到表格里
        ui_->log_table->setItem(i, 0, item0);
        ui_->log_table->setItem(i, 1, item1);
        ui_->log_table->setItem(i, 2, item2);
        ui_->log_table->setItem(i, 3, item3);
        ui_->log_table->setItem(i, 4, item4);

        // 设置内容居中
        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
        item2->setTextAlignment(Qt::AlignCenter);
        item3->setTextAlignment(Qt::AlignCenter);
        item4->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::addOrderSuccess() {
    loading_->hide();
    QMessageBox::information(this, tr("添加成功"), tr("订单添加成功"));
    refreshOrderTab();
}
void MainWindow::addOrderFailure() {
    loading_->hide();
    QMessageBox::warning(this, tr("添加失败"), tr("订单添加失败"));
}

void MainWindow::on_order_group_box_fileDropped(const QString &path) {
    QString temp_path = QCoreApplication::applicationDirPath() + "/temp";
    auto    dest_path = create_folder(temp_path) + "/" + QFileInfo(path).fileName();

    // 查看是否为 .zip .7z 文件
    if (path.endsWith(".zip")) {
        if (!utils::Utils::decompressionZipFile(path.toStdString(), dest_path.toStdString())) {
            QMessageBox::warning(this, tr("解压失败"), tr("请检查压缩文件是否损坏"));
            return;
        }
    }

    auto file_name  = QFileInfo(path).fileName().mid(0, QFileInfo(path).fileName().lastIndexOf("."));
    auto splits     = file_name.split(" ");
    auto order_name = splits[0];
    ui_->order_name_line->setText(order_name);

    Directory dir(dest_path.toStdString());
    auto      files = dir.files();

    for (auto &file : *files) {
        if (file.name().find("内盒") != std::string::npos && file.extension() == ".csv") {
            ui_->box_file_path_line->setText(QString::fromStdString(file.path()));
        } else if (file.name().find("inner") != std::string::npos && file.extension() == ".csv") {
            ui_->box_file_path_line->setText(QString::fromStdString(file.path()));
        }

        if (ui_->box_file_path_line->text().isEmpty()) {
            if (file.name().find("内盒") != std::string::npos && file.extension() == ".xlsx") {
                ui_->box_file_path_line->setText(QString::fromStdString(file.path()));
            } else if (file.name().find("inner") != std::string::npos && file.extension() == ".xlsx") {
                ui_->box_file_path_line->setText(QString::fromStdString(file.path()));
            }
        }

        if (file.name().find("外箱") != std::string::npos && file.extension() == ".csv") {
            ui_->carton_file_path_line->setText(QString::fromStdString(file.path()));
        } else if (file.name().find("outer") != std::string::npos && file.extension() == ".csv") {
            ui_->carton_file_path_line->setText(QString::fromStdString(file.path()));
        }

        if (ui_->carton_file_path_line->text().isEmpty()) {
            if (file.name().find("外箱") != std::string::npos && file.extension() == ".xlsx") {
                ui_->carton_file_path_line->setText(QString::fromStdString(file.path()));
            } else if (file.name().find("outer") != std::string::npos && file.extension() == ".xlsx") {
                ui_->carton_file_path_line->setText(QString::fromStdString(file.path()));
            }
        }

        if (file.name().find("单卡") != std::string::npos && file.extension() == ".csv") {
            ui_->card_file_path_line->setText(QString::fromStdString(file.path()));
        }

        if (ui_->card_file_path_line->text().isEmpty()) {
            if (file.name().find("单卡") != std::string::npos && file.extension() == ".xlsx") {
                ui_->card_file_path_line->setText(QString::fromStdString(file.path()));
            }
        }
    }
}

void MainWindow::authSuccess(const std::string &confirm_by) {
    auth_->hide();
    auth_->clearEdit();

    auto order_name = ui_->log_order_name_combo->currentText().toStdString();
    auto type_idx   = ui_->log_scan_type_combo->currentIndex();

    OrderDao::Type type;
    switch (type_idx) {
    case 0:
        type = OrderDao::Type::BOX;
        break;
    case 1:
        type = OrderDao::Type::CARTON;
        break;
    case 2:
        type = OrderDao::Type::CARD;
        break;
    }

    if (order_dao_->confirm(order_name, confirm_by, type)) {
        QMessageBox::information(this, tr("确认成功"), tr("订单确认成功"));
        refreshLogTab();
    } else {
        QMessageBox::warning(this, tr("确认失败"), tr("订单确认失败"));
    }
}

void MainWindow::authFailure(const QString &message) {
    auth_->hide();
    auth_->clearEdit();
    QMessageBox::warning(this, tr("确认失败"), message);
}

void MainWindow::exportExcelSuccess() {
    loading_->hide();
    QMessageBox::information(this, tr("导出成功"), tr("导出成功"));
}
void MainWindow::exportExcelFailure() {
    loading_->hide();
    QMessageBox::warning(this, tr("导出失败"), tr("导出失败"));
}

void MainWindow::init_window() {
    // 设置窗口标题
    setWindowTitle(tr("条码比对系统 v3.2.0           当前用户:   ") + QString::fromStdString(user_dao_->currentUser()->description));
}

void MainWindow::init_ui() {
    // 初始化内盒比对 tab
    init_box_tab();

    // 初始化外箱比对 tab
    init_carton_tab();

    // 初始化卡片比对 tab
    init_card_tab();

    // 初始化订单管理 tab
    init_order_tab();

    // 初始化用户管理 tab
    init_user_tab();

    // 初始化日志管理 tab
    init_log_tab();
}

void MainWindow::init_dao() {
    role_dao_   = RoleDaoFactory::create(sqlite_db_, mysql_db_);
    mode_dao_   = ModeDaoFactory::create(sqlite_db_, mysql_db_);
    order_dao_  = OrderDaoFactory::create(sqlite_db_, mysql_db_);
    format_dao_ = FormatDaoFactory::create(sqlite_db_, mysql_db_);
}

void MainWindow::init_box_tab() {
    ui_->box_order_name_combo->setEditable(true);
    ui_->box_order_name_combo->lineEdit()->setPlaceholderText(tr("请输入订单号"));
    ui_->box_order_name_combo->lineEdit()->setAlignment(Qt::AlignCenter);

    // 下拉框模糊匹配
    QStringList order_name_list;
    for (auto &order : order_dao_->all()) {
        order_name_list.append(QString::fromStdString(order->name));
    }

    QCompleter *completer = new QCompleter(order_name_list, this);
    completer->setFilterMode(Qt::MatchContains);
    ui_->box_order_name_combo->setCompleter(completer);

    // 设置内盒数据状态下拉框内容
    QStringList box_data_status = {tr("全部"), tr("未扫描"), tr("已扫描")};
    ui_->box_datas_status_comb_box->addItems(box_data_status);
    ui_->box_datas_status_comb_box->setEditable(true);
    ui_->box_datas_status_comb_box->lineEdit()->setAlignment(Qt::AlignCenter);
    ui_->box_datas_status_comb_box->lineEdit()->setReadOnly(true);

    QStringList box_header = {tr("内盒起始条码"), tr("内盒结束条码"), tr("首卡条码"), tr("尾卡条码")};
    init_table(ui_->box_table, box_header, box_header.size());
}

void MainWindow::init_carton_tab() {
    ui_->carton_order_name_combo->setEditable(true);
    ui_->carton_order_name_combo->lineEdit()->setPlaceholderText(tr("请输入订单号"));
    ui_->carton_order_name_combo->lineEdit()->setAlignment(Qt::AlignCenter);

    // 下拉框模糊匹配
    QStringList order_name_list;
    for (auto order : order_dao_->all()) {
        order_name_list.append(QString::fromStdString(order->name));
    }

    QCompleter *completer = new QCompleter(order_name_list, this);
    completer->setFilterMode(Qt::MatchContains);
    ui_->carton_order_name_combo->setCompleter(completer);

    QStringList carton_data_status = {tr("全部"), tr("未扫描"), tr("已扫描")};
    ui_->carton_datas_status_comb_box->addItems(carton_data_status);
    ui_->carton_datas_status_comb_box->setEditable(true);
    ui_->carton_datas_status_comb_box->lineEdit()->setAlignment(Qt::AlignCenter);
    ui_->carton_datas_status_comb_box->lineEdit()->setReadOnly(true);

    QStringList carton_header = {tr("外箱起始条码"), tr("外箱结束条码"), tr("内盒起始或结束条码")};
    init_table(ui_->carton_table, carton_header, carton_header.size());
}

void MainWindow::init_card_tab() {
    ui_->card_order_name_combo->setEditable(true);
    ui_->card_order_name_combo->lineEdit()->setPlaceholderText(tr("请输入订单号"));
    ui_->card_order_name_combo->lineEdit()->setAlignment(Qt::AlignCenter);

    // 下拉框模糊匹配
    QStringList order_name_list;
    for (auto order : order_dao_->all()) {
        order_name_list.append(QString::fromStdString(order->name));
    }

    QCompleter *completer = new QCompleter(order_name_list, this);
    completer->setFilterMode(Qt::MatchContains);
    ui_->card_order_name_combo->setCompleter(completer);

    QStringList card_data_status = {tr("全部"), tr("未扫描"), tr("已扫描")};
    ui_->card_datas_status_comb_box->addItems(card_data_status);
    ui_->card_datas_status_comb_box->setEditable(true);
    ui_->card_datas_status_comb_box->lineEdit()->setAlignment(Qt::AlignCenter);
    ui_->card_datas_status_comb_box->lineEdit()->setReadOnly(true);

    QStringList card_header = {tr("内盒起始条码"), tr("内盒结束条码")};
    init_table(ui_->card_table, card_header, card_header.size());
}

void MainWindow::init_order_tab() {
    QStringList order_header = {tr("订单号"), tr("校验格式"), tr("条码模式"), tr("创建时间")};
    init_table(ui_->order_table, order_header, order_header.size());
}

void MainWindow::init_user_tab() {
    QStringList user_header = {tr("用户名"), tr("中文名"), tr("权限")};
    init_table(ui_->user_table, user_header, user_header.size());
    // 设置权限下拉框内容
    for (auto &role : role_dao_->all()) {
        ui_->selected_combo_box->addItem(QString::fromStdString(role->description));
    }
}

void MainWindow::init_log_tab() {
    QStringList log_header = {tr("盒号/箱号"), tr("起始条码"), tr("扫描详情"), tr("操作人"), tr("操作时间")};
    init_table(ui_->log_table, log_header, log_header.size());

    // 下拉框模糊匹配
    QStringList order_name_list;
    for (auto &order : order_dao_->all()) {
        order_name_list.append(QString::fromStdString(order->name));
    }

    QCompleter *completer = new QCompleter(order_name_list, this);
    completer->setFilterMode(Qt::MatchContains);
    ui_->log_order_name_combo->setCompleter(completer);

    QStringList log_scan_types = {tr("内盒比对"), tr("外箱比对"), tr("卡片比对")};
    ui_->log_scan_type_combo->addItems(log_scan_types);

    QStringList log_status = {tr("全部"), tr("已扫描"), tr("未扫描")};
    ui_->log_status_combo->addItems(log_status);
}

void MainWindow::init_table(QTableWidget *table, QStringList header, int col_count) {
    table->setRowCount(0);            // 设置行数
    table->setColumnCount(col_count); // 设置列数
    table->setHorizontalHeaderLabels(header);

    // 设置表格样式
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);   // 禁止编辑
    table->setSelectionBehavior(QAbstractItemView::SelectRows);  // 选中整行
    table->setSelectionMode(QAbstractItemView::SingleSelection); // 只能选中一行
    table->setAlternatingRowColors(true);                        // 隔行变色

    // 设置表头字体
    QFont header_font;
    header_font.setPointSize(12);
    header_font.setBold(true);
    table->horizontalHeader()->setFont(header_font);
    table->verticalHeader()->setFont(header_font);

    // 设置表头内容颜色
    table->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
    table->verticalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");

    // 设置表格字体
    QFont font;
    font.setPointSize(12);
    table->setFont(font);

    // 设置自动调整列宽,并可以通过拖动表头来调整列宽
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置表格内容居中
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    // 设置表格内容颜色
    table->setStyleSheet("selection-background-color:lightblue;"); // 选中行背景色
    table->setStyleSheet("background-color:transparent;");         // 背景透明
    table->setStyleSheet("alternate-background-color:lightgray;"); // 隔行变色
    table->setStyleSheet("color:black;");                          // 字体颜色

    // 设置表格边框
    table->setFrameShape(QFrame::NoFrame);                 // 无边框
    table->setShowGrid(false);                             // 不显示网格线
    table->setStyleSheet("border:none;");                  // 边框颜色
    table->setStyleSheet("gridline-color:rgba(0,0,0,0);"); // 网格线颜色
}

void MainWindow::init_signals_slots() {
    // 工具栏 - 用户
    connect(ui_->switch_user_action, &QAction::triggered, this, &MainWindow::switchUserActionTriggered);

    // 工具栏 - 语言
    connect(ui_->chinese_action, &QAction::triggered, this, &MainWindow::chineseActionTriggered);
    connect(ui_->english_action, &QAction::triggered, this, &MainWindow::englishActionTriggered);

    // 工具栏 - 配置
    connect(ui_->set_tag_data_action, &QAction::triggered, this, &MainWindow::setTagDataActionTriggered);

    // 切换 tab
    connect(ui_->tabWidget, &QTabWidget::currentChanged, [=](int index) {
        switch (index) {

        // 内盒比对
        case 0:
            refreshBoxTab();
            break;

        // 外箱比对
        case 1:
            refreshCartonTab();
            break;

        // 卡片比对
        case 2:
            refreshCardTab();
            break;

        // 订单管理
        case 3:
            refreshOrderTab();
            break;

        // 用户管理
        case 4:
            refreshUserTab();
            break;

        // 日志管理
        case 5:
            refreshLogTab();
            break;

        default:
            break;
        }
    });

    // 内盒比对 Tab
    connect(ui_->box_order_name_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::boxSelectOrder);
    connect(ui_->box_datas_status_comb_box, &QComboBox::currentTextChanged, this, &MainWindow::selectBoxDatasStatus);
    connect(ui_->box_start_or_end_line, &QLineEdit::returnPressed, this, &MainWindow::toCardStartBarcode);
    connect(ui_->card_start_line, &QLineEdit::returnPressed, this, &MainWindow::toCardEndBarcode);
    connect(ui_->card_end_line, &QLineEdit::returnPressed, this, &MainWindow::compareBox);

    // 外箱比对 Tab
    connect(ui_->carton_order_name_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::cartonSelectOrder);
    connect(ui_->carton_table, &QTableWidget::itemSelectionChanged, this, &MainWindow::showSelectedCarton);
    connect(ui_->carton_datas_status_comb_box, &QComboBox::currentTextChanged, this, &MainWindow::selectCartonDatasStatus);
    connect(ui_->carton_start_line, &QLineEdit::returnPressed, this, &MainWindow::toCartonEndBarcode);
    connect(ui_->carton_end_line, &QLineEdit::returnPressed, this, &MainWindow::toTargetBarcode);
    connect(ui_->target_line, &QLineEdit::returnPressed, this, &MainWindow::compareCarton);

    // 卡片比对 Tab
    connect(ui_->card_order_name_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::cardSelectOrder);
    connect(ui_->card_rescanned_btn, &QPushButton::clicked, this, &MainWindow::cardResccenedBtnClicked);
    connect(ui_->card_table, &QTableWidget::itemSelectionChanged, this, &MainWindow::showSelectedCard);
    connect(ui_->card_datas_status_comb_box, &QComboBox::currentTextChanged, this, &MainWindow::selectCardDatasStatus);
    connect(ui_->card_box_start_or_end_line, &QLineEdit::returnPressed, this, &MainWindow::toCardLabelBarcode);
    connect(ui_->card_label_line, &QLineEdit::returnPressed, this, &MainWindow::compareCardLabel);
    connect(ui_->card_stop_label_btn, &QPushButton::clicked, this, &MainWindow::toCardBarcode);
    connect(ui_->card_line, &QLineEdit::returnPressed, this, &MainWindow::compareCard);

    // 订单管理 Tab
    connect(ui_->select_box_file_ptn, &QPushButton::clicked, this, &MainWindow::selectBoxFileBtnClicked);
    connect(ui_->select_carton_file_ptn, &QPushButton::clicked, this, &MainWindow::selectCartonFileBtnClicked);
    connect(ui_->select_card_file_ptn, &QPushButton::clicked, this, &MainWindow::selectCardFileBtnClicked);
    connect(ui_->order_table, &QTableWidget::itemSelectionChanged, this, &MainWindow::showSelectedOrder);
    connect(ui_->add_order_btn, &QPushButton::clicked, this, &MainWindow::addOrderBtnClicked);
    connect(ui_->update_order_btn, &QPushButton::clicked, this, &MainWindow::updateOrderBtnClicked);
    connect(ui_->remove_order_btn, &QPushButton::clicked, this, &MainWindow::removeOrderBtnClicked);
    connect(ui_->clear_order_btn, &QPushButton::clicked, this, &MainWindow::clearOrderBtnClicked);
    connect(ui_->order_group_box, &OrderGroupBox::fileDropped, this, &MainWindow::on_order_group_box_fileDropped);

    // 用户管理 Tab
    connect(ui_->user_table, &QTableWidget::itemSelectionChanged, this, &MainWindow::showSelectedUser);
    connect(ui_->update_user_btn, &QPushButton::clicked, this, &MainWindow::updateUserBtnClicked);
    connect(ui_->add_user_btn, &QPushButton::clicked, this, &MainWindow::addUserBtnClicked);
    connect(ui_->remove_user_btn, &QPushButton::clicked, this, &MainWindow::removeUserBtnClicked);
    connect(ui_->clear_user_btn, &QPushButton::clicked, this, &MainWindow::clearUserBtnClicked);

    // 日志管理 Tab
    connect(ui_->log_search_btn, &QPushButton::clicked, this, &MainWindow::searchLogBtnClicked);
    connect(ui_->log_qc_confirm_btn, &QPushButton::clicked, this, &MainWindow::qcConfirmBtnClicked);
    connect(ui_->log_export_excel_btn, &QPushButton::clicked, this, &MainWindow::exportExacelBtnClicked);
    connect(auth_.get(), &Auth::authSuccess, this, &MainWindow::authSuccess);
    connect(auth_.get(), &Auth::authFailure, this, &MainWindow::authFailure);
}

void MainWindow::switch_language(const QString &language_file) {
    qApp->removeTranslator(&translator_);

    if (translator_.load(":/translation/" + language_file + ".qm")) {
        qApp->installTranslator(&translator_);
        ui_->retranslateUi(this);
    } else {
        qDebug() << "加载语言失败：" << language_file;
    }
}

bool MainWindow::log(const QString &filename, const QString &msg) {

    // 获取当前时间
    QString current_date_time = QString::fromStdString(utils::Utils::now());

    // 获取当前程序所在路径
    QString current_path = QCoreApplication::applicationDirPath();

    // 获取日志文件路径
    QString log_path = current_path + "/" + filename;
    create_folder(log_path.mid(0, log_path.lastIndexOf('/')));

    // 打开日志文件
    QFile file(log_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return false;
    }

    // 写入日志
    QTextStream text_stream(&file);
    text_stream << current_date_time << " " << msg << "\r\n";
    file.flush();
    file.close();

    return true;
}

int MainWindow::scroll_to_value(QTableWidget *table, const QString &value, bool selected) {

    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem *item = table->item(row, col);
            if (item && item->text() == value) { // 精确匹配

                if (selected)
                    table->selectRow(row); // 选中整行
                else
                    table->clearSelection(); // 清除选择

                table->scrollToItem(item, QAbstractItemView::PositionAtCenter); // 滚动到目标
                return row + 1;
            }
        }
    }

    return -1;
}

void MainWindow::clear_box_compare_group_layout(QLayout *layout) {
    if (!layout) return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            delete w; // 删除控件
        }
        if (QLayout *childLayout = item->layout()) {
            clear_box_compare_group_layout(childLayout); // 递归清理子布局
        }
        delete item; // 删除 layoutItem
    }
}

QString MainWindow::create_folder(const QString &folder_path) {
    QDir dir(folder_path);
    if (dir.exists(folder_path)) {
        return folder_path;
    }

    QString parentDir = create_folder(folder_path.mid(0, folder_path.lastIndexOf('/')));
    QString dirName   = folder_path.mid(folder_path.lastIndexOf('/') + 1);

    QDir parentPath(parentDir);
    if (!dirName.isEmpty()) {
        parentPath.mkpath(dirName);
    }
    return parentDir + "/" + dirName;
}