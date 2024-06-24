'use server'

import { connection } from "@/db/mysql"


interface BdData {
    pid: number;
    process_name: string;
    ratio: string;
}
export interface PieData {
    pid: number;
    process_name: string;
    ratio: number;
}
 
export async function fetchPieData() {

    const query = `
    SELECT
        pid,
        process_name,
        CASE
            WHEN sum(if(memory_requests.is_mmap, size, -size)) < 0 THEN 0
            ELSE sum(if(memory_requests.is_mmap, size, -size))
        END
        / (
            SELECT
                CASE
                    WHEN sum(if(memory_requests.is_mmap, size, -size)) <= 0 THEN 1
                    ELSE sum(if(memory_requests.is_mmap, size, -size))
                END
            FROM memory_requests
        ) AS ratio
    FROM memory_requests
    GROUP BY pid, process_name
    ORDER BY ratio DESC
    LIMIT 10;
    `;

    const [rows] = await connection.query(query) as unknown as [BdData[]]
    const formatedData = rows.map((row) => ({
        ...row,
        ratio: Number(Number(row.ratio).toFixed(2))
    }))

    if (formatedData.length === 10) {
        formatedData[9].process_name = "Otros"
        formatedData[9].pid = 0
        formatedData[9].ratio = Number(Number(1 - formatedData.reduce((acc, row) => acc + row.ratio, 0)).toFixed(2))
    }

    return formatedData;
}
