
'use server'

import { connection } from "@/db/mysql"

export interface UtilizationInfo {
    id: number;
    pid: number;
name: string;
size: string;
percentage: string;
virtualMemory: number;
}
 
export async function getPercentage(page: number) : Promise<{items: UtilizationInfo[], pages: number, page: number, virtualMemory: number}> {

    const query = `
    with all_percentages as (
        SELECT
            pid,
            process_name,
            CASE
                WHEN sum(if(memory_requests.is_mmap, size, -size)) < 0 THEN 0
                ELSE sum(if(memory_requests.is_mmap, size, -size))
            END as size,
            CASE
                WHEN sum(if(memory_requests.is_mmap, size, -size)) < 0 THEN 0
                ELSE sum(if(memory_requests.is_mmap, size, -size))
            END
            / (
                select amount*1024 from virtual_memory order by id desc limit 1
            ) AS ratio
        FROM memory_requests
        GROUP BY pid, process_name
        ORDER BY pid
    ) select 
        pid as id,
        pid,
        process_name as name,
        size,
        case when ratio > 100 then ratio % 100 else ratio end as percentage
    from all_percentages limit 15 offset ${(page - 1) * 15};
    `;
    const [rows] = await connection.query(query) as any;

    const query2 = `
    SELECT count(*) as total FROM (
        SELECT
            pid,
            process_name,
            CASE
                WHEN sum(if(memory_requests.is_mmap, size, -size)) < 0 THEN 0
                ELSE sum(if(memory_requests.is_mmap, size, -size))
            END as size,
            CASE
                WHEN sum(if(memory_requests.is_mmap, size, -size)) < 0 THEN 0
                ELSE sum(if(memory_requests.is_mmap, size, -size))
            END
            / (
                select amount from virtual_memory order by id desc limit 1
            ) AS ratio
        FROM memory_requests
        GROUP BY pid, process_name
    ) as t;
    `;
    const [rows2] = await connection.query(query2) as any;
    const items = rows.map((row: any) => ({
        ...row,
        percentage: `${Number(row.percentage*100).toFixed(2)}%`,
        size: `${(row.size / 1024 / 1024).toFixed(2)} MB`,
    }));
    const total = Number(rows2[0]?.['total'] || 0);
    const pages = Math.ceil(total / 15);


    const query3 = `
    select amount from virtual_memory order by id desc limit 1;
    `;
    const [rows3] = await connection.query(query3) as any;
    const virtualMemory = rows3[0]?.['amount'] || 0;


    return {items, pages, page, virtualMemory};
}
