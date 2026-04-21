ATTACH DATABASE 'D:\XH Software\品质部\BARCODE\box_data_2.db' AS db2;
ATTACH DATABASE 'D:\XH Software\品质部\BARCODE\box_data_3.db' AS db3;

DROP TABLE IF EXISTS merged_IMO000224;

CREATE TABLE merged_IMO000224 AS
SELECT 
    t1.*,
    CAST(
        MAX(
            COALESCE(t1.status, 0),
            COALESCE(t2.status, 0),
            COALESCE(t3.status, 0)
        ) AS INTEGER
    ) AS status_all
FROM IMO000224 t1
LEFT JOIN db2.IMO000224 t2 
    ON t1.id = t2.id
LEFT JOIN db3.IMO000224 t3
    ON t1.id = t3.id;