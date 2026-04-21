#pragma once

#include <string>

struct CardData {
    int         id;
    std::string card_number;   // card number of the card
    std::string iccid;         // iccid of the card
    std::string imsi;          // imsi of the card
    int         quantity;      // quantity of the carton
    std::string iccid_barcode; // barcode of the iccid
    std::string imsi_barcode;  // barcode of the imsi
    int         status;        // scan status of thecarton, 0: not scanned, 1: scanned, 2: scanned with error
    std::string scanned_by;    // name of the person who scanned the card
    std::string scanned_at;    // date and time when the card was scanned
};