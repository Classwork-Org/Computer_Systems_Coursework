#!/usr/bin/python
import re
import pprint
import csv
pp = pprint.PrettyPrinter(indent=4)

def write_data_to_csv_file(filename, data):
    index = {}
    index['parameter value'] = 0
    index['Simulation Time'] = 1
    index['Total Customers'] = 2
    index['Turned Away Customers'] = 3
    index['Average Time In Seat'] = 4
    index['Average people in the shop'] = 5
    index['Average Time Spent In Shop'] = 6

    headers = ['value', 'Simulation Time', 'Total Customers', 'Turned Away Customers', 'Average Time In Seat', 'Average people in the shop', 'Average Time Spent In Shop']

    def addnewline(count):
        for i in range(0, count):
            writer.writerow([''])

    with open(filename, 'w') as aggregate_metrics:
        writer = csv.writer(aggregate_metrics)
        for (parameter, results) in data.items():
            writer.writerow([parameter])
            writer.writerow(headers)
            for (value, metric) in results:
                row = len(headers)*['']
                row[index['parameter value']] = value
                for (each_metric, result) in metric:
                    row[index[each_metric]] = result
                writer.writerow(row)

def load_data_from_metrics_file(filename):
    with open(filename, 'r') as metric_file:
        data = {}
        parameter = ''
        value = 0
        run_results = []
        first_blob = True
        for line in metric_file:
            if line.startswith('METRIC'):
                if not first_blob:
                    if parameter in data.keys():
                        data[parameter].append((value, run_results))
                    else:
                        data[parameter] = []
                    run_results =  []
                else:
                    first_blob = False
                match = re.search(r'METRIC: (\w+) ([0-9]+)', line)
                if match is not None:
                    (parameter, value) = match.groups()
            elif line.startswith('INFO'):
                match = re.search(r'INFO: ([\w ]+): ([0-9\.]+)', line)
                if match is not None:
                    (metric, result) = match.groups()
                    run_results.append((metric, result))
        return data

def main():
    data = load_data_from_metrics_file('metrics.data')
    write_data_to_csv_file('aggregate_metrics.csv', data)

if __name__ == "__main__":
    main()