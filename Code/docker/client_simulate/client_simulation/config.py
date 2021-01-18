from configparser import ConfigParser

def configDTB(filename='client_simulation/database.ini', section='postgresql'):
    parser=ConfigParser()
    parser.read(filename)

    db={}
    if parser.has_section(section):
        params = parser.items(section)
        # print(params)
        print(params[0][1])
        print('???')
        for param in params:
            # print(param[0])
            # print(param[1])
            db[param[0]] = param[1]
    else:
        raise Exception('Section {0} not found in the {1} file'.format(section, filename))

    return db

def configMQTT(filename='client_simulation/mqtt.ini', section='mosquitto'):
    parser=ConfigParser()
    parser.read(filename)

    mqtt={}
    if parser.has_section(section):
        params = parser.items(section)
        for param in params:
            mqtt[param[0]] = param[1]
    else:
        raise Exception('Section {0} not found in the {1} file'.format(section, filename))

    return mqtt
    