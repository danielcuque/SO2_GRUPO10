import { Pagination, Skeleton, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow } from '@nextui-org/react'
import { use, useEffect, useRef, useState } from 'react'
import { AllocInfo, getLast } from './getLast';

export const LastAllocs = () => {

  const [page, setPage] = useState(1);
    const [data, setData] = useState<{
        items: AllocInfo[],
        pages: number,
    }>({
        items: [],
        pages: 1,
    });
    const intervalRef = useRef<NodeJS.Timeout>();

    const getKeyValue = (item :any, key:any) => {
        return item[key]
    }

    const fetchData = async (page:number) => {
        console.log('refresh: ', {page})
        const resp = await getLast(page);
        setData(resp);
    }

    useEffect(() => {
        fetchData(page)
        intervalRef.current = setInterval(() => fetchData(page), 1000);
        return () => clearInterval(intervalRef.current);
    }, [page]);

  return (
    <Table
      aria-label="Example table with client side pagination"
      bottomContent={
        <div className="flex w-full justify-center">
          <Pagination
            isCompact
            showControls
            showShadow
            color="secondary"
            page={page}
            total={data.pages}
            onChange={(page) => setPage(page)}
          />
        </div>
      }
      classNames={{
        wrapper: "min-h-[222px]",
      }}
    >
      <TableHeader>
        <TableColumn key="pid">PID</TableColumn>
        <TableColumn key="call">Llamada</TableColumn>
        <TableColumn key="size">Tamaño</TableColumn>
        <TableColumn key="date">Fecha</TableColumn>
      </TableHeader>
      <TableBody items={data.items}>
        {(item) => (
          <TableRow key={item.id}>
            {
            (columnKey) => <TableCell>
                {getKeyValue(item, columnKey)}
            </TableCell>
            }
          </TableRow>
        )}
      </TableBody>
    </Table>
  )
}
