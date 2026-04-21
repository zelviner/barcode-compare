#pragma once

#include "database/user/user_dao.h"
#include "ui_auth.h"

#include <memory>
#include <qmainwindow>
#include <zel/myorm.h>

class Auth : public QMainWindow {
    Q_OBJECT

  public:
    Auth(const std::shared_ptr<UserDao> &user_dao, QMainWindow *parent = nullptr);
    ~Auth();

    /// @brief 确认按钮点击事件
    void confirmBtnClicked();

    /// @brief 取消按钮点击事件
    void cancelBtnClicked();

    void clearEdit();

  signals:
    // 信号函数，用于向外界发射信号
    void authSuccess(const std::string &confirm_by);
    void authFailure(const QString &message);

  private:
    /// @brief 初始化窗口
    void init_window();

    /// @brief 初始化UI
    void init_ui();

    /// @brief 初始化信号槽
    void init_signals_slots();

  private:
    Ui_Auth                 *ui_;
    std::shared_ptr<UserDao> user_dao_;
};
