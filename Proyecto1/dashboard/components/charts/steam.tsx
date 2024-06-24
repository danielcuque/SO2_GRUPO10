'use client'
import React, { useEffect, useRef, useState } from "react";
import Chart from "react-apexcharts";
import { PieData, fetchPieData } from "./pie-data";


export const Steam = () => {

  const [data, setData] = useState<PieData[]>([]);
  const intervalRef = useRef<NodeJS.Timeout>();

  const fetchData = async () => {
    const data = await fetchPieData();
    setData(data);
  }

  const series = data.map((d) => d.ratio*100)
  const labels = data.map((d) => `${d.process_name} - ${d.pid}`)

  useEffect(() => {
    fetchData();
    intervalRef.current = setInterval(fetchData, 1000);
    return () => clearInterval(intervalRef.current);
  }, []);

  return (
    <>
      <div className="w-full z-20">
        <div id="chart">
          <Chart 
            type="pie"
            series={series}
            options={{
              labels,
              legend: {
                labels: {
                  colors: "#fff",
                },
              },
            }}
            height={425} />
        </div>
      </div>
    </>
  );
};
