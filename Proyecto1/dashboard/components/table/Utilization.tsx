import { Pagination,Table, TableBody, TableCell, TableColumn, TableHeader, TableRow } from '@nextui-org/react'
import { useEffect, useRef, useState } from 'react'
import { UtilizationInfo, getPercentage } from './getPercentage';

export const Utilization = () => {

    const [page, setPage] = useState(1);
    const [data, setData] = useState<{
        items: UtilizationInfo[],
        pages: number,
        virtualMemory: number
    }>({
        items: [],
        pages: 1,
        virtualMemory: 0
    });
    const intervalRef = useRef<NodeJS.Timeout>();

    const getKeyValue = (item :any, key:any) => {
        return item[key]
    }

    const fetchData = async (page:number) => {
        const resp = await getPercentage(page);
        setData(resp);
    }

    useEffect(() => {
        fetchData(page)
        intervalRef.current = setInterval(() => fetchData(page), 1000);
        return () => clearInterval(intervalRef.current);
    }, [page]);

  return (
    <>
    <h5 className="text-2xl font-bold text-center">
      Total de memoria virtual: {(((Number(data.virtualMemory) || 0))/1024).toFixed(2)} MB
    </h5>
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
        <TableColumn key="name">Nombre</TableColumn>
        <TableColumn key="size">Tamaño</TableColumn>
        <TableColumn key="percentage">Porcentaje Utilización</TableColumn>
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
    </>
  )
}
