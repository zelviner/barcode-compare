#pragma once

#include <qstring>

struct CartonInfo {
    QString carton_start_barcode; // 外箱起始条码
    QString carton_end_barcode;   // 外箱结束条码
    QString target_barcode;       // 目标条码
};
