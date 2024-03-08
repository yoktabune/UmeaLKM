import socket
import subprocess
import struct
import fcntl

SERVER_IP = "172.20.10.4"
PORT = 30303
BUFFER_SIZE = 4096
CREDENTIALS_FILE = "credentials.txt"

def check_credentials(username, password):
    with open(CREDENTIALS_FILE, 'r') as f:
        for line in f:
            stored_username, stored_password = line.strip().split(',')
            if stored_username == username and stored_password == password:
                return True
    return False

def change_password(username, new_password):
    with open(CREDENTIALS_FILE, 'r') as f:
        lines = f.readlines()
    with open(CREDENTIALS_FILE, 'w') as f:
        for line in lines:
            stored_username, _ = line.strip().split(',')
            if stored_username == username:
                f.write(f"{username},{new_password}\n")
            else:
                f.write(line)
def run_c_program(args):
    try:
        # C programını argümanlarla birlikte çalıştır
        process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        stdout, stderr = process.communicate()
        if process.returncode == 0:
            return stdout
        else:
            return f"Error: {stderr}"
    except Exception as e:
        return f"Exception: {str(e)}"
def run_dmesg():
    # `dmesg` komutunu çalıştır ve çıktısını yakala
    process = subprocess.Popen(['dmesg'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    stdout, stderr = process.communicate()
    
    # Filtreleme: Sadece 'my_device' içeren satırları al
    filtered_output = "\n".join(line for line in stdout.split("\n") if 'mydevice' in line)
    
    return filtered_output


def handle_client(conn, addr):
    print(f"[NEW CONNECTION] Connected to {addr}")

    conn.send("Username: ".encode('utf-8'))
    username = conn.recv(BUFFER_SIZE).decode('utf-8').strip()
    conn.send("Password: ".encode('utf-8'))
    password = conn.recv(BUFFER_SIZE).decode('utf-8').strip()

    if not check_credentials(username, password):
        conn.send("Authentication Failed.\n".encode('utf-8'))
        conn.close()
        return
    else :
        conn.send("Authentication Completed.\n".encode('utf-8'))
    
    connected = True
    while connected:
        data = conn.recv(BUFFER_SIZE).decode('utf-8')
        data=data.rstrip("\n")
        print(f"[{addr}] {data}")
        if not data:
            break

        args = ['./directcommand'] + data.split(' ')

        # Komutun başlangıcına göre uygun C programı çağrısı yap ve çıktıyı gönder
        if data.startswith("w") or data.startswith("r") or data.startswith("s"):
            _output = run_c_program(args)
            response = f"output of hashtable:\n {_output}"
            conn.send(response.encode('utf-8'))
        elif data.startswith("get_logs"):
            _output=run_dmesg()
            response = f"output of kernel logs:\n {_output}"
            conn.send(response.encode('utf-8'))
        elif data.startswith("change_password"):
            _, new_password = data.split(' ')
            change_password(username, new_password)
            conn.send("Password changed successfully.\n".encode('utf-8'))
            continue
        else:
            conn.send("Unknown command.\n".encode('utf-8'))

    conn.close()

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_sock:
        server_sock.bind((SERVER_IP, PORT))
        server_sock.listen()
        print(f"Server listening on {SERVER_IP}:{PORT}")
        while True:
            conn, addr = server_sock.accept()
            handle_client(conn, addr)

if __name__ == "__main__":
    start_server()