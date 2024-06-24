"use client";
import React from "react";
import dynamic from "next/dynamic";
import { LastAllocs } from "../table/LastAllocs";
import { Utilization } from "../table/Utilization";


const Chart = dynamic(
  () => import("../charts/steam").then((mod) => mod.Steam),
  {
    ssr: false,
  }
);

export const Content = () => (
  <div className="h-full lg:px-6 flex flex-col gap-y-2 p-3">

        {/* Chart */}
        <div className="flex flex-col gap-2 mt-2">
          <h3 className="text-xl font-semibold">Memoria Solicitada Por Proceso</h3>
          <div className="w-full bg-default-50 shadow-lg rounded-2xl p-6 ">
            <Chart />
          </div>
        </div>

        <div className="flex flex-col gap-2 mt-2">
          <h3 className="text-xl font-semibold">Utilización de Memoria</h3>
          <div className="w-full bg-default-50 shadow-lg rounded-2xl p-6 ">
            <Utilization />
          </div>
        </div>

        <div className="flex flex-col gap-2 mt-2">
          <h3 className="text-xl font-semibold">Últimas Solicitudes</h3>
          <div className="w-full bg-default-50 shadow-lg rounded-2xl p-6 ">
            <LastAllocs />
          </div>
        </div>
  </div>
);
