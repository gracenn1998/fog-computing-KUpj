# msg = "Hello world"
# print(msg)

import psycopg2
import config
from datetime import datetime
import time
import paho.mqtt.client as mqtt 
import json



params = config.configDTB()
conn = psycopg2.connect(**params)
reqCnt = {}
#need fix later
reqCnt["1"] = 0
ir = {}
peakAnalyze= {}

# def on_message(client, userdata, message):



if __name__ == '__main__':
    print('helu')
    deviceID = input("please enter a device id: ")#"simulatedC1"
    params = config.configMQTT()
    client = mqtt.Client(deviceID)
    pubDataChannel = "/pulseoximeter"
    pubReqChannel =  "/monitor/request"
    reqMsg = json.dumps({"req" : "on", "targetID" : deviceID})
    client.connect(params['host'])
    # client.on_message = on_message
    client.publish(pubReqChannel, reqMsg)
    client.subscribe("/monitor/respond/" + deviceID)
    cur = conn.cursor()
    cnt =0
    curtime = pretime = time.time()

    while 1 :
        client.loop_start()
        read_sql = """ SELECT value->'ir'
                    FROM sensor_reading
                    WHERE time >= '2020-01-19 00:28:19' and time <= '2020-01-19 00:28:26'
        """
        # try:
        cur.execute(read_sql)
        rows = cur.fetchall()
        for row in rows:
            print(row)
            record = {}
            val = {}
            record["did"] = deviceID
            record["stype"] = 1
            val['ir'] = row[0]
            record['val'] = val
            msg = json.dumps(record)
            print('msg', msg)
            while curtime-pretime<0.01: #interval 10millis
                curtime = time.time()
            pretime = curtime
            client.publish(pubDataChannel, msg)
        client.loop_stop()        
        

    