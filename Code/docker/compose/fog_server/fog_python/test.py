# msg = "Hello world"
# print(msg)

import psycopg2
import config
from datetime import datetime
import time
import paho.mqtt.client as mqtt
import json

def connectDTB():
    conn = None
    try:
        params = config.configDTB()
        print('Connecting...')
        conn = psycopg2.connect(**params)

        cur = conn.cursor()
        print('PostgreSQL version: ')
        cur.execute('Select version()')
        db_version = cur.fetchone()
        print(db_version)

        cur.close()
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()
            print('Connection closed')


def create_hypertable():
    commands = (
        """
        CREATE TABLE conditions2 (
            time        TIMESTAMPTZ         NOT NULL,
            location    TEXT                NOT NULL,
            temp        DOUBLE PRECISION    null,
            humid       DOUBLE PRECISION    NULL
        );
        """,
        """
        SELECT create_hypertable('conditions2', 'time');
        """
    )
    conn = None
    try:
        params = config.configDTB()
        conn = psycopg2.connect(**params)
        cur = conn.cursor()
        for command in commands:
            cur.execute(command)
        cur.close()
        conn.commit()
    except(Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()


def insert_data():
    now = datetime.now()
    sql =   """ INSERT INTO conditions(time, location, temp, humidity, light)
                VALUES (%s, %s, %s, %s, %s)
                RETURNING time;
            """
    record = (now, 'office', 15.5, 46.5, 123.1)
    time = 0

    try: 
        params = config.configDTB()
        conn = psycopg2.connect(**params)
        cur = conn.cursor()
        cur.execute(sql, record)
        time = cur.fetchone()[0]
        conn.commit()
        cur.close()
    except(Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()

    return time

def on_message(client, userdata, message):
    print("message received " ,str(message.payload.decode("utf-8")))
    print("message topic=",message.topic)
    # print("message qos=",message.qos)
    # print("message retain flag=",message.retain)
    # print(message.payload)
    mes = json.loads(message.payload.decode("utf-8"))
    print('ò e í e')
    # # print(mes['value1'])
    val_json = json.dumps(mes['val'])
    print('ò e í e2')
    print(val_json)
    # # val_json = json.dumps()
    did = mes['did']
    # print(did)
    pid = '1'  #mes['pid'] or get data from somewhere else?
    sensor_type = mes['stype']
    # print(sensor_type)
    # ts = mes['timesptamp']

    if(message.topic == "/pulseoximeter"):
        print('ò e í e')
        now = datetime.now()
        sql =   """ INSERT INTO sensor_reading(time, patient_id, device_id, sensortype_id, value)
                    VALUES (%s, %s, %s, %s, %s)
                """
        record = (now, pid, did, sensor_type, val_json)

        # now = datetime.now()
        # sql =   """ INSERT INTO sensor_reading(time, patient_id, device_id, sensortype_id)
        #             VALUES (%s, %s, %s, %s)
        #         """
        # record = (now, '1', '1', '1')

        try: 
            print('ò e í e')
            params = config.configDTB()
            conn = psycopg2.connect(**params)
            cur = conn.cursor()
            cur.execute(sql, record)
            # time = cur.fetchone()[0]
            conn.commit()
            cur.close()
        except(Exception, psycopg2.DatabaseError) as error:
            print(error)
        finally:
            if conn is not None:
                conn.close()


def connectMQTT():
    params = config.configMQTT()
    client = mqtt.Client("test")
    # # try:d
    client.connect(params['host'])
    # except (Exception e):
    #     print(e)
    # client.publish("/test","ON")
    client.on_message = on_message

    client.loop_start()
    client.subscribe("/pulseoximeter")
    # client.publish("/test", "12.3")
    time.sleep(4)
    client.loop_stop()

if __name__ == '__main__':
    params = config.configMQTT()
    client = mqtt.Client("test")
    # # try:
    client.connect(params['host'])
    client.on_message = on_message
    client.subscribe("/pulseoximeter")
    while 1 :
        client.loop_start()
        
        # client.publish("/test", "12.3")
        # time.sleep(4)
        client.loop_stop()
    # connectDTB()