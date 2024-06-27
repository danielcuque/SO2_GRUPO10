
'use server'

import { connection } from "@/db/mysql"

export interface AllocInfo {
    id: number;
    pid: number;
    call: 'mmap' | 'munmap';
    size: string; // in MB
    date: string; // YYYY-MM-DD HH:MM:SS
}
 
export async function getLast(page: number) : Promise<{items: AllocInfo[], pages: number, page: number}> {

    const query = `
    SELECT
        id,
        pid,
        is_mmap,
        size,
        unix_time
    FROM memory_requests
    ORDER BY id DESC
    LIMIT 15
    OFFSET ${(page - 1) * 15};
    `;

    const [rows] = await connection.query(query) as any;

    const formatedData = rows.map((row: any) => {
        return {
            id: row.id,
            pid: row.pid,
            call: row.is_mmap ? 'mmap' : 'munmap',
            size: `${(row.size / 1024 / 1024).toFixed(2)} MB`,
            date: new Date(row.unix_time * 1000).toISOString().replace('T', ' ').replace(/\.\d+Z/, '')
        }
    })

    const query2 = `
    SELECT
        COUNT(*) as total
    FROM memory_requests;
    `;
    const [rows2] = await connection.query(query2) as any;
    const total = Number(rows2[0]?.['total'] || 0);
    const pages = Math.ceil(total / 15);

    return {items: formatedData, pages, page};
}
