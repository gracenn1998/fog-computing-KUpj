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

def on_message(client, userdata, message):
    mes = json.loads(message.payload.decode("utf-8"))
    val_json = json.dumps(mes['val'])
    did = mes['did']
    pid = '1'  #mes['pid'] or get data from somewhere else?
    sensor_type = mes['stype']
    
    if(message.topic == "/pulseoximeter"):
        now = datetime.now()
        sql =   """ INSERT INTO sensor_reading(time, patient_id, device_id, sensortype_id, value)
                    VALUES (%s, %s, %s, %s, %s)
                """
        record = (now, pid, did, sensor_type, val_json)

        try: 
            cur = conn.cursor()
            cur.execute(sql, record)
            # time = cur.fetchone()[0]
            conn.commit()
            cur.close()
        except(Exception, psycopg2.DatabaseError) as error:
            print(error)

if __name__ == '__main__':
    params = config.configMQTT()
    client = mqtt.Client("test")
    client.connect(params['host'])
    client.on_message = on_message
    client.subscribe("/pulseoximeter")
    while 1 :
        client.loop_start()
        client.loop_stop()