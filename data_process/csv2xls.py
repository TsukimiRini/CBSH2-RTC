import enum
import xlwt
import sys
import os
import csv
from collections import namedtuple
from enum import Enum

size = int(sys.argv[1])
a_num = {
    10: [4, 8, 16, 32],
    20: [4, 8, 16, 32, 64],
    40: [4, 8, 16, 32, 64, 128],
    80: [4, 8, 16, 32, 64, 128, 256],
    160: [4, 8, 16, 32, 64, 128, 256, 512]
}
num_list = a_num[size]


class Row(Enum):
    instance_name = 0,
    solver_name = 1,
    success = 2,
    agent_num = 3,
    success_agent_cnt = 4,
    sum_of_costs = 5,
    makespan = 6,
    runtime = 7


data_dir = "../outputs"
# success_rate = 0
# success_case = []
# complete_rate = []
# sum_of_costs = []


def getIter(name):
    iter = name.split('.')[-2].split('_')[-1]
    return int(iter)


def getStats(a_cnt):
    success_rate = 0
    success_case = []
    complete_rate = []
    sum_of_costs = []
    makespan = []
    runtime = []

    success_cnt = 0
    stats_dir = os.path.join(data_dir, "stats")
    listdir = os.listdir(stats_dir)
    for file in listdir:
        if file.split('_')[2] == str(size) + '.csv' and int(
                file.split('_')[0]) == a_cnt:
            with open(os.path.join(stats_dir, file), "r") as fd:
                f_csv = csv.reader(fd)
                # Row = namedtuple('Row', next(f_csv))
                headers = next(f_csv)
                for row in f_csv:
                    # row_info = Row(*row)
                    success_case.append([
                        row[Row.instance_name.value[0]],
                        row[Row.success.value[0]] == "yes"
                    ])
                    if row[Row.success.value[0]] == "yes":
                        complete_rate.append([
                            row[Row.instance_name.value[0]],
                            float(row[Row.success_agent_cnt.value[0]]) /
                            float(row[Row.agent_num.value[0]])
                        ])
                    else:
                        complete_rate.append([
                            row[Row.instance_name.value[0]],
                            (float(row[Row.success_agent_cnt.value[0]]) - 1) /
                            float(row[Row.agent_num.value[0]])
                        ])
                    sum_of_costs.append([
                        row[Row.instance_name.value[0]],
                        int(row[Row.sum_of_costs.value[0]])
                    ])
                    makespan.append([
                        row[Row.instance_name.value[0]],
                        int(row[Row.makespan.value[0]])
                    ])
                    runtime.append([
                        row[Row.instance_name.value[0]],
                        float(row[Row.runtime.value])
                    ])
                    if row[Row.success.value[0]] == "yes":
                        success_cnt += 1
                success_rate = float(success_cnt) / len(success_case)
    return success_rate, success_case, complete_rate, sum_of_costs, makespan, runtime


# # success rate
# wb = xlwt.Workbook(encoding='utf-8')
# newsheet = wb.add_sheet('success stats', cell_overwrite_ok=True)
# newsheet.write(row, 0, "scene name")
# newsheet.write(row, 1, "success")
all = {
    "success_rate": [],
    "success_case": [],
    "complete_rate": [],
    "sum_of_costs": [],
    "makespan": [],
    "runtime": [],
}
row = 0
wb = xlwt.Workbook(encoding='utf-8')

# success rate
newsheet = wb.add_sheet('success stats', cell_overwrite_ok=True)
newsheet.write(row, 0, "agent num")
newsheet.write(row + 1, 0, "success_rate")
for idx, num in enumerate(num_list):
    success_rate, success_case, complete_rate, sum_of_costs, makespan, runtime = getStats(
        num)
    all["success_rate"].append(success_rate)
    all["success_case"].append(success_case)
    all["complete_rate"].append(complete_rate)
    all["sum_of_costs"].append(sum_of_costs)
    all["makespan"].append(makespan)
    all["runtime"].append(runtime)
    newsheet.write(row, idx + 1, num)
row += 1
for idx, rate in enumerate(all["success_rate"]):
    newsheet.write(row, idx + 1, rate)

# complete_rate
row = 0
newsheet = wb.add_sheet('completion stats', cell_overwrite_ok=True)
for i in range(1, 101):
    newsheet.write(i, 0, i - 1)
sum = 0
success_cnt = 0
for idx, num in enumerate(num_list):
    sum = 0
    success_cnt = 0
    newsheet.write(row, idx + 1, num)
    for iter, a_case in enumerate(all["complete_rate"][idx]):
        # if not all["success_case"][idx][iter][1]:
        #     newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, a_case[1])
        #     continue
        success_cnt += 1
        sum += a_case[1]
        newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, a_case[1])
    newsheet.write(101, 0, "Average")
    newsheet.write(101, idx + 1, float(sum) / success_cnt)

# sum_of_costs
row = 0
newsheet = wb.add_sheet('sum of costs', cell_overwrite_ok=True)
for i in range(1, 101):
    newsheet.write(i, 0, i - 1)
sum = 0
success_cnt = 0
for idx, num in enumerate(num_list):
    sum = 0
    success_cnt = 0
    newsheet.write(row, idx + 1, num)
    for iter, a_case in enumerate(all["sum_of_costs"][idx]):
        if not all["success_case"][idx][iter][1]:
            newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, "fail")
            continue
        success_cnt += 1
        sum += a_case[1]
        newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, a_case[1])
    newsheet.write(101, 0, "Average")
    newsheet.write(101, idx + 1, float(sum) / success_cnt)

# makespan
row = 0
newsheet = wb.add_sheet('makespan', cell_overwrite_ok=True)
for i in range(1, 101):
    newsheet.write(i, 0, i - 1)
sum = 0
success_cnt = 0
for idx, num in enumerate(num_list):
    sum = 0
    success_cnt = 0
    newsheet.write(row, idx + 1, num)
    for iter, a_case in enumerate(all["makespan"][idx]):
        if not all["success_case"][idx][iter][1]:
            newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, "fail")
            continue
        success_cnt += 1
        sum += a_case[1]
        newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, a_case[1])
    newsheet.write(101, 0, "Average")
    newsheet.write(101, idx + 1, float(sum) / success_cnt)

# runtime
row = 0
newsheet = wb.add_sheet('runtime', cell_overwrite_ok=True)
for i in range(1, 101):
    newsheet.write(i, 0, i - 1)
sum = 0
all_sum = 0
success_cnt = 0
for idx, num in enumerate(num_list):
    sum = 0
    all_sum = 0
    success_cnt = 0
    newsheet.write(row, idx + 1, num)
    for iter, a_case in enumerate(all["runtime"][idx]):
        all_sum += a_case[1]
        if not all["success_case"][idx][iter][1]:
            newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, a_case[1])
            continue
        success_cnt += 1
        sum += a_case[1]
        newsheet.write(row + getIter(a_case[0]) + 1, idx + 1, a_case[1])
    newsheet.write(101, 0, "Success Average")
    newsheet.write(101, idx + 1, str(float(sum) / success_cnt))
    newsheet.write(102, 0, "All Average")
    newsheet.write(102, idx + 1,
                   str(float(all_sum) / len(all["success_case"][idx])))

wb.save('../outputs/results_{}.xls'.format(size))