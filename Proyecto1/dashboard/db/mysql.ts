import mysql from "mysql2/promise";

export const connection = mysql.createPool({
    host: "roundhouse.proxy.rlwy.net",
    user: "root",
    password: "fIfkYvtSvlmpSptMUpqxrmtBemKzoizk",
    database: "so2p1",
    port: 37220,
});