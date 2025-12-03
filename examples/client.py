import socket
import time

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client_socket.connect(('127.0.0.1', 12346))

for i in range(5):
    client_socket.sendall(b'Hello, server!')
    response = client_socket.recv(1024)
    print(f"Получен ответ от сервера: {response}")
    time.sleep(1)

client_socket.close()
