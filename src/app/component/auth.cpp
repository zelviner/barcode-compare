#include "auth.h"

#include <memory>
#include <qcompleter>
#include <qdir>
#include <qfile>
#include <qmessagebox>
#include <qsettings>
#include <zel/core.h>

using namespace zel::utility;

Auth::Auth(const std::shared_ptr<UserDao> &user_dao, QMainWindow *parent)
    : QMainWindow(parent)
    , ui_(new Ui_Auth)
    , user_dao_(user_dao) {
    ui_->setupUi(this);

    init_window();

    init_ui();

    init_signals_slots();
}

Auth::~Auth() { delete ui_; }

void Auth::init_window() {
    // 设置窗口标题
    setWindowTitle(tr("品质确认"));
}

void Auth::init_ui() {}

void Auth::init_signals_slots() {
    // 回车键
    connect(ui_->password_edit, &QLineEdit::returnPressed, this, &Auth::confirmBtnClicked);

    // 确认按钮
    connect(ui_->confirm_btn, &QPushButton::clicked, this, &Auth::confirmBtnClicked);

    // 取消按钮
    connect(ui_->cancel_btn, &QPushButton::clicked, this, &Auth::cancelBtnClicked);
}

void Auth::confirmBtnClicked() {
    // 获取用户名和密码
    std::string entered_name     = ui_->username_edit->text().toStdString();
    std::string entered_password = ui_->password_edit->text().toStdString();

    if (entered_name.empty() || entered_password.empty()) {
        authFailure(tr("用户名或密码不能为空"));
        return;
    }

    // 品质确认
    for (auto &user : user_dao_->all()) {
        if (user->name == entered_name && user->password == entered_password) {
            if (user->role_id != 3) {
                authFailure(tr("请使用品质人员账号进行确认"));
                return;
            } else {
                authSuccess(user->description);
                return;
            }
        }
    }
    authFailure(tr("用户名或密码错误"));
}

void Auth::cancelBtnClicked() {
    this->hide();
    this->clearEdit();
}

void Auth::clearEdit() {
    ui_->username_edit->clear();
    ui_->password_edit->clear();
}
