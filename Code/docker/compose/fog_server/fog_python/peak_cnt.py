#!/usr/bin/env python
# Implementation of algorithm from https://stackoverflow.com/a/22640362/6029703
import numpy as np
import pylab

import config
import psycopg2


params = config.configDTB()
conn = psycopg2.connect(**params)

def thresholding_algo(y, lag, threshold, influence):
    signals = np.zeros(len(y))
    filteredY = np.array(y)
    avgFilter = [0]*len(y)
    stdFilter = [0]*len(y)
    avgFilter[lag - 1] = np.mean(y[0:lag])
    stdFilter[lag - 1] = np.std(y[0:lag])
    for i in range(lag, len(y)):
        if abs(y[i] - avgFilter[i-1]) > threshold * stdFilter [i-1]:
            if y[i] > avgFilter[i-1]:
                signals[i] = 1
            else:
                signals[i] = -1

            filteredY[i] = influence * y[i] + (1 - influence) * filteredY[i-1]
            avgFilter[i] = np.mean(filteredY[(i-lag+1):i+1])
            stdFilter[i] = np.std(filteredY[(i-lag+1):i+1])
        else:
            signals[i] = 0
            filteredY[i] = y[i]
            avgFilter[i] = np.mean(filteredY[(i-lag+1):i+1])
            stdFilter[i] = np.std(filteredY[(i-lag+1):i+1])

    return dict(signals = np.asarray(signals),
                avgFilter = np.asarray(avgFilter),
                stdFilter = np.asarray(stdFilter))

ir = []
cur = conn.cursor()
read_sql = """ SELECT value->'ir'
                    FROM sensor_reading
                    WHERE time >= '2020-01-19 00:28:19' and time <= '2020-01-19 00:28:26'
    """
# read_sql = """ SELECT value->'ir'
#                             FROM sensor_reading
#                             WHERE device_id = 1
#                             ORDER BY TIME DESC LIMIT 1000
#             """

try:
    cur.execute(read_sql)
    rows = cur.fetchall()
    print('ir val cnt: ', cur.rowcount)
    # print(rows)
    # print('.')
    # print(rows[1][0])
    for i in range(0, 30):
        ir.append(0)
    for row in rows:
        ir.append(row[0])
    #     print(row)
    # print(ir)
    cur.close()
except (Exception, psycopg2.DatabaseError) as error:
    print(error)
finally:
    if conn is not None:
        conn.close()

# Data
y = np.array(ir)

# Settings: lag = 30, threshold = 5, influence = 0
lag = 30
threshold = 2.5
influence = 0.3

# Run algo with settings from above
result = thresholding_algo(y, lag=lag, threshold=threshold, influence=influence)

# Plot result
pylab.subplot(211)
pylab.plot(np.arange(1, len(y)+1), y)

pylab.plot(np.arange(1, len(y)+1),
           result["avgFilter"], color="cyan", lw=2)

pylab.plot(np.arange(1, len(y)+1),
           result["avgFilter"] + threshold * result["stdFilter"], color="green", lw=2)

pylab.plot(np.arange(1, len(y)+1),
           result["avgFilter"] - threshold * result["stdFilter"], color="green", lw=2)

pylab.subplot(212)
pylab.step(np.arange(1, len(y)+1), result["signals"], color="red", lw=2)
pylab.ylim(-1.5, 1.5)
# print(result["signals"][0])
cnt=0
pre_res = 0
for res in result["signals"]:
    if(res==1):
        if(pre_res == 0):
            cnt+=1
        pre_res = 1    
    else: pre_res = 0
print(cnt)
pylab.show()
