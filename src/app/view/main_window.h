#pragma once

#include "app/component/auth.h"
#include "app/component/box_widget.h"
#include "app/component/card_widget.h"
#include "app/component/loading.h"
#include "database/format/format_dao.h"
#include "database/mode/mode_dao.h"
#include "database/order/order_dao.h"
#include "database/role/role_dao.h"
#include "database/user/user_dao.h"
#include "ui_main_window.h"

#include <memory>
#include <qmainwindow>
#include <qtranslator>
#include <queue>
#include <zel/myorm.h>

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow(const std::shared_ptr<SQLite::Database> &sqlite_db, const std::shared_ptr<zel::myorm::Database> &mysql_db,
               const std::shared_ptr<UserDao> &user_dao, QMainWindow *parent = nullptr);
    ~MainWindow();

    /// @brief 工具栏操作
    void switchUserActionTriggered(); // 切换用户动作触发
    void chineseActionTriggered();    // 切换中文动作触发
    void englishActionTriggered();    // 切换英文动作触发
    void setTagDataActionTriggered(); // 设置标签数据动作触发

    /// @brief 内盒比对 Tab
    void boxSelectOrder();                                                  // 内盒选择订单
    void selectBoxDatasStatus();                                            // 选择内盒数据状态
    void toCardStartBarcode();                                              // 获取首卡条码
    void toCardEndBarcode();                                                // 获取尾卡条码
    void compareBox();                                                      // 比对内盒
    void refreshBoxTab();                                                   // 刷新内盒比对 Tab
    void refreshBoxTable(const std::string &order_name, const int &status); // 刷新内盒表格

    /// @brief 外箱比对 Tab
    void cartonSelectOrder();                                                                       // 外箱选择订单
    void showSelectedCarton();                                                                      // 显示选中外箱
    void selectCartonDatasStatus();                                                                 // 选择外箱数据状态
    void toTargetBarcode();                                                                         // 获取目标条码
    void compareCarton();                                                                           // 比对外箱
    void refreshCartonTab();                                                                        // 刷新外箱比对 Tab
    void refreshCartonTable(const std::string &order_name, const int &status);                      // 刷新外箱表格
    void refreshBoxCompareGroup(const int &cols, const std::string &selected_carton_start_barcode); // 刷新内盒比对组

    /// @brief 卡片比对 Tab
    void cardSelectOrder();                                                                       // 卡片选择订单
    void showSelectedCard();                                                                      // 显示选中卡片
    void selectCardDatasStatus();                                                                 // 选择卡片数据状态
    void cardResccenedBtnClicked();                                                               // 重新扫描按钮点击
    void toCardLabelBarcode();                                                                    // 获取标签条码
    void compareCardLabel();                                                                      // 比对标签
    void toCardBarcode();                                                                         // 获取卡片条码
    void compareCard();                                                                           // 比对卡片
    void refreshCardTab();                                                                        // 刷新卡片比对 Tab
    void refreshCardTable(const std::string &order_name, const int &status);                      // 刷新卡片表格
    void refreshCardCompareGroup(const int &cols, const std::string &selected_box_start_barcode); // 刷新卡片比对组

    /// @brief 订单管理 Tab
    void selectBoxFileBtnClicked();    // 选择内盒文件按钮点击事件
    void selectCartonFileBtnClicked(); // 选择外箱文件按钮点击事件
    void selectCardFileBtnClicked();   // 选择单卡文件按钮点击事件
    void addOrderBtnClicked();         // 添加订单按钮点击事件
    void updateOrderBtnClicked();      // 修改订单按钮点击事件
    void removeOrderBtnClicked();      // 删除订单按钮点击事件
    void clearOrderBtnClicked();       // 清空订单按钮点击事件
    void showSelectedOrder();          // 显示选中订单
    void refreshOrderTab();            // 刷新订单管理 tab

    /// @brief 用户管理 Tab
    void addUserBtnClicked();    // 添加用户按钮点击事件
    void updateUserBtnClicked(); // 修改用户按钮点击事件
    void removeUserBtnClicked(); // 删除用户按钮点击事件
    void clearUserBtnClicked();  // 清空用户按钮点击事件
    void showSelectedUser();     // 显示选中用户
    void refreshUserTab();       // 刷新用户管理 Tab

    /// @brief 日志管理 Tab
    void searchLogBtnClicked();                                                              // 搜索日志按钮点击事件
    void qcConfirmBtnClicked();                                                              // 品质确认按钮点击事件
    void exportExacelBtnClicked();                                                           // 导出 Excel 按钮点击事件
    void refreshLogTab();                                                                    // 刷新日志管理 Tab
    void refreshLogTable(const std::string &order_name, const int &type, const int &status); // 刷新日志表格
    void refreshBoxLogTable(const std::string &order_name, const int &status);               // 刷新内盒日志表格
    void refreshCartonLogTable(const std::string &order_name, const int &status);            // 刷新外箱日志表格
    void refreshCardLogTable(const std::string &order_name, const int &status);              // 刷新卡片日志表格

  private slots:
    void addOrderSuccess();
    void addOrderFailure();

    void on_order_group_box_fileDropped(const QString &path);

    void authSuccess(const std::string &confirm_by);
    void authFailure(const QString &message);

    void exportExcelSuccess();
    void exportExcelFailure();

  private:
    /// @brief 初始化窗口
    void init_window();

    /// @brief 初始化 UI
    void init_ui();

    /// @brief 初始化数据库
    void init_dao();

    /// @brief 初始化内盒比对 Tab
    void init_box_tab();

    /// @brief 初始化外箱比对 Tab
    void init_carton_tab();

    /// @brief 初始化卡片比对 Tab
    void init_card_tab();

    /// @brief 初始化订单管理 Tab
    void init_order_tab();

    /// @brief 初始化用户管理 Tab
    void init_user_tab();

    /// @brief 初始化日志管理 Tab
    void init_log_tab();

    /// @brief 初始化表格
    void init_table(QTableWidget *table, QStringList header, int col_count);

    /// @brief 初始化信号槽
    void init_signals_slots();

    /// @brief 切换语言
    void switch_language(const QString &language_file);

    /// @brief 记录日志
    bool log(const QString &filename, const QString &msg);

    int scroll_to_value(QTableWidget *table, const QString &value, bool selected = true);

    void clear_box_compare_group_layout(QLayout *layout);

    /// @brief 创建文件夹
    /// @param folder_path  文件夹路径
    QString create_folder(const QString &folder_path);

  private:
    Ui_MainWindow           *ui_;
    QTranslator              translator_;
    std::shared_ptr<Loading> loading_;
    std::shared_ptr<Auth>    auth_;
    std::queue<BoxWidget *>  box_widgets_;
    std::queue<CardWidget *> card_widgets_;
    std::queue<CardWidget *> card_label_widgets_;
    std::queue<QString>      card_label_barcodes_;
    int                      current_box_number_;

    std::shared_ptr<SQLite::Database>     sqlite_db_;
    std::shared_ptr<zel::myorm::Database> mysql_db_;
    std::shared_ptr<RoleDao>              role_dao_;
    std::shared_ptr<UserDao>              user_dao_;
    std::shared_ptr<ModeDao>              mode_dao_;
    std::shared_ptr<OrderDao>             order_dao_;
    std::shared_ptr<FormatDao>            format_dao_;
};
