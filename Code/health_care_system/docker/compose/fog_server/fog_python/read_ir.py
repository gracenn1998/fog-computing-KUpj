# msg = "Hello world"
# print(msg)

import config
import psycopg2
import peak_cnt2
import peak_rt


params = config.configDTB()
conn = psycopg2.connect(**params)

if __name__ == '__main__':
    cnt =0
    cur = conn.cursor()
    read_sql = """ SELECT value->'ir'
                    FROM sensor_reading
                    WHERE time >= '2020-01-19 00:28:10' and time <= '2020-01-19 00:28:20'
    """
    try:
        cur.execute(read_sql)
        rows = cur.fetchall()
        print('ir val cnt: ', cur.rowcount)
        ir = []
        for row in rows:
            ir.append(row[0])
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)

    peakcnt = peak_rt.real_time_peak_detection(ir, 30, 2.5, 0.5)

    read_sql = """ SELECT value->'ir'
                    FROM sensor_reading
                    WHERE time > '2020-01-19 00:28:20' and time <= '2020-01-19 00:28:30'
    """
    try:
        cur.execute(read_sql)
        rows = cur.fetchall()
        print('ir val cnt: ', cur.rowcount)
        # print(rows)
        # print('.')
        # print(rows[1][0])
        cnt = 0
        pre_signal = 0
        signal = 0
        for row in rows:
            signal = peakcnt.thresholding_algo(row[0])
            if(signal==1):
                if(pre_signal == 0):
                    cnt+=1
                    print("beep")
                    pre_signal = 1
            else: pre_signal = 0
        print("cnt: ", cnt)
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)

    # signals = peak_cnt2.peak_detection_smoothed_zscore_v2(ir, 30, 2.5, 0.5)["signals"]
    # # print(signals)
    # for res in signals:
    #     if(res==1):
    #         if(pre_res == 0):
    #             cnt+=1
    #         pre_res = 1    
    #     else: pre_res = 0
    # print('peaks: ', cnt)
    # while 1 :
    #     client.loop_start()
    #     client.loop_stop()