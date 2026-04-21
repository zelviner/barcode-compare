#pragma once

#include <string>

struct BoxData {
    int         id;
    std::string filename;      // prd data filename
    std::string box_number;    // box number of the box
    std::string start_number;  // start number of the box
    std::string end_number;    // end number of the box
    int         quantity;      // quantity of the box
    std::string start_barcode; // start barcode of the box
    std::string end_barcode;   // end barcode of the box
    int         status;        // scan status of the box, 0: not scanned, 1: scanned, 2: scanned with error
    int         card_status;   // card status of the box, 0: not scanned, 1: scanned, 2: scanned with error
    int         carton_status; // carton status of the box, 0: not packed, 1: packed, 2: packed with error
    std::string scanned_by;    // name of the person who scanned the box
    std::string scanned_at;    // date and time when the box was scanned
};