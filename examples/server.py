import time
import zmq

def print_data():
    with open('C:/Users/Ариша/OneDrive/Рабочий стол/visual.txt', 'r') as f:
        print(f.read())

context = zmq.Context()
server = context.socket(zmq.REP)

server.bind("tcp://172.20.10.18:5555")
server.RCVTIMEO = 2000

print("Сервер запущен и ожидает подключений...")
file = open('C:/Users/Ариша/OneDrive/Рабочий стол/visual.txt', 'a')
count = 0

try:
    while 1:
        try:
            data = server.recv()
            print(f"Получены данные: {data}")
            count += 1
            file.write(f"{count} : {data}\n")
            server.send(b'Hello, from Server!')
            print(f"Количество полученных пакетов: {count}")
        except zmq.Again:
            continue
except KeyboardInterrupt:
    print('Сервер завершил работу')
    print_data()
finally:
    file.close()
    server.close()
    context.term()
