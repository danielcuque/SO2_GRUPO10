
'use server'

import { connection } from "@/db/mysql"

export interface UtilizationInfo {
    id: number;
    pid: number;
name: string;
size: string;
percentage: string;
}
 
export async function getPercentage(page: number) : Promise<{items: UtilizationInfo[], pages: number, page: number}> {

    const query = `
    SELECT
        id,
        pid,
        process_name as name,
        size,
        1 as percentage
    FROM memory_requests
    ORDER BY id DESC
    LIMIT 15
    OFFSET ${(page - 1) * 15};
    `;

    const [rows] = await connection.query(query) as any;

    const query2 = `
    SELECT
        COUNT(*) as total
    FROM memory_requests;
    `;
    const [rows2] = await connection.query(query2) as any;
    const items = rows.map((row: any) => ({
        ...row,
        size: `${(row.size / 1024 / 1024).toFixed(2)} MB`,
    }));
    const total = Number(rows2[0]['total']);
    const pages = Math.ceil(total / 15);

    return {items, pages, page};
}
