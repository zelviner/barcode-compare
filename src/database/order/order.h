#pragma once

#include <qstring>

struct Order {
    int         id;                     // 订单ID
    std::string name;                   // 订单名称
    std::string check_format;           // 校验格式
    int         carton_start_check_num; // 外箱开始校验位数
    int         carton_end_check_num;   // 外箱结束校验位数
    int         box_start_check_num;    // 内盒开始校验位数
    int         box_end_check_num;      // 内盒结束校验位数
    int         card_start_check_num;   // 卡片开始校验位数
    int         card_end_check_num;     // 卡片结束校验位数
    int         box_scanned_num;        // 已扫描内盒数量
    int         carton_scanned_num;     // 已扫描外箱数量
    int         mode_id;                // 条码模式
    std::string box_file_path;          // 内盒文件路径
    std::string carton_file_path;       // 外箱文件路径
    std::string card_file_path;         // 单卡文件路径
    std::string create_at;              // 创建时间
    std::string box_confirm_by;         // 内盒确认人
    std::string box_confirm_at;         // 内盒确认时间
    std::string carton_confirm_by;      // 外箱确认人
    std::string carton_confirm_at;      // 外箱确认时间
    std::string card_confirm_by;        // 卡片确认人
    std::string card_confirm_at;        // 卡片确认时间
};