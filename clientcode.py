import socket

SERVER_IP = "172.20.10.4"
PORT = 30303

def send_command(s, command):
    """Server'a komut gönder ve yanıtını al."""
    s.sendall(command.encode())
    response = s.recv(1024).decode("utf-8")
    print("Received:", response)

if __name__ == "__main__":
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_IP, PORT))
        username = input("Enter username: ")
        password = input("Enter password: ")
        s.sendall(username.encode())
        response = s.recv(1024)
        s.sendall(password.encode())
        response = s.recv(1024).decode("utf-8")
        response = s.recv(1024).decode("utf-8")
        if "Fail" in response:
            print(response)
        else:
            while True:
                command = input("Enter command \n(w and key and value for WRITE ,\nr and key for READ ,\ns for SAVE,\nget_logs for Kernel logs for hashtable,\nchange_password and new_password for Changing Password\n EXIT to quit): ")
                if command.upper() == "EXIT":
                    break
                send_command(s, command)