import subprocess
import sys
import PySimpleGUI as sg
from enum import Enum
import argparse
import socket
import threading
import zeep

msgSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
global web_url 
web_url = "http://localhost:8000/?wsdl"


def readMessage(sock):
    """Esta función se encarga de leer los mensajes que llegan al socket de mensajes.
        Recibe el socket de mensajes, va leyendo byte a byte hasta encontrarse '/0' y devuelve el mensaje leído."""
    
    message = ""
    while True:
        c = sock.recv(1)
        if c == b'\0':
            break
        message += c.decode('utf-8')
    return message


def listen(dummy, window):
    """Esta función se encarga de escuchar los mensajes que llegan al socket de mensajes.
        Se lee el código de operación y se ejecuta la acción correspondiente."""
    while True:
        try:
            server, ip_server = msgSocket.accept()
            
            op = readMessage(server)
            if op == "SEND_MESSAGE": 
                user_alias = readMessage(server)
                id_message = readMessage(server)
                message = readMessage(server)
                window['_SERVER_'].print("s> MESSAGE " + id_message + " FROM " + user_alias + "\n" + message + "\nEND")
                
                server.close()
            elif op == "SEND_MESS_ACK":
                id_ack = readMessage(server)
                window['_SERVER_'].print("s> SEND MESSAGE " + id_ack + " OK")

                server.close()
                       
        except:
            print("listen end")
            return


class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _quit = 0
    _username = None
    _alias = None
    _date = None
    
    # ******************** METHODS *******************

    # *
    # * @param user - User name to register in the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def register(user, window):
        """Esta función se encarga de registrar al usuario en el servidor.
            Recibe el nombre de usuario y la ventana de la interfaz gráfica.
            Creará un socket y se conectará al servidor, enviando el código de operación y la información necesaria."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        

        try:
            sock.connect((client._server, client._port))

            sock.sendall(b'REGISTER\0')
            sock.sendall(bytes(client._username + '\0', 'utf-8'))
            sock.sendall(bytes(user + '\0', 'utf-8'))
            sock.sendall(bytes(client._date + '\0', 'utf-8'))
        except :
            window['_SERVER_'].print("s> REGISTER FAIL") 

        else: 
            #Recibe la respuesta del servidor y, dependiendo del código de respuesta, muestra el mensaje por pantalla.
            response = int(sock.recv(1).decode('utf-8'))
            match response:
                case 0:
                    window['_SERVER_'].print("s> REGISTER OK")
                    return
                case 1:
                    window['_SERVER_'].print("s> USERNAME IN USE")
                    return
                case 2:
                    window['_SERVER_'].print("s> REGISTER FAIL")
                    return
            return
                
        finally:
            sock.close()
        
        return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 *
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def unregister(user, window):
        """Esta función se encarga de eliminar al usuario en el servidor.
            Recibe el nombre de usuario y la ventana de la interfaz gráfica.
            Creará un socket y se conectará al servidor, enviando el código de operación y la información necesaria."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            sock.connect((client._server, client._port))

            sock.sendall(b'UNREGISTER\0')
            sock.sendall(bytes(user + '\0', 'utf-8'))
        except :
            window['_SERVER_'].print("s> UNREGISTER FAIL")
        
        else:
            #Recibe la respuesta del servidor y, dependiendo del código de respuesta, muestra el mensaje por pantalla.
            response = int(sock.recv(1).decode('utf-8'))
            match response:
                case 0:
                    window['_SERVER_'].print("s> UNREGISTER OK")
                    return
                case 1:
                    window['_SERVER_'].print("s> USER DOES NOT EXIST")
                    return
                case  2:
                    window['_SERVER_'].print("s> UNREGISTER FAIL")
                    return
        finally:
            sock.close()
        return client.RC.ERROR


    # *
    # * @param user - User name to connect to the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def connect(user, window):
        print("connect")
        """Esta función se encarga de conectar al usuario en el servidor.
            Recibe el nombre de usuario y la ventana de la interfaz gráfica.
            Creará un socket y se conectará al servidor, enviando el código de operación y la información necesaria.
            Además, creará un socket de mensajes para recibir los mensajes que le envíen.
            Se creará un hilo para escuchar los mensajes que lleguen al socket de mensajes."""
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            sock.connect((client._server, client._port))
           
            # Buscamos un puerto libre para el socket de mensajes y lo enlazamos
            msgSocket.bind((client._server, 0))
            msgSocket.listen(1)
            print("Listening on port " + str(msgSocket.getsockname()[1]))
           
            # Creamos un hilo para escuchar los mensajes que lleguen al socket de mensajes
            hilo = threading.Thread(target= listen, args=("a",window))
            hilo.start()
             
            sock.sendall(b'CONNECT\0')
            sock.sendall(bytes(user + '\0', 'utf-8'))
            sock.sendall(bytes(str(msgSocket.getsockname()[1]) + '\0', 'utf-8'))
        
        except :
            window['_SERVER_'].print("s> CONNECT FAIL")
        
        else:
            #Recibe la respuesta del servidor y, dependiendo del código de respuesta, muestra el mensaje por pantalla.
            response = int(sock.recv(1).decode('utf-8'))
            match response:
                case 0:
                    window['_SERVER_'].print("s> CONNECT OK")
                    return
                case 1:
                    window['_SERVER_'].print("s> CONNECT FAIL, USER DOES NOT EXIST")
                    return
                case 2:
                    window['_SERVER_'].print("s> USER ALREADY CONNECTED")
                    return
                case 3:
                    window['_SERVER_'].print("s> CONNECT FAIL")
                    return

        return client.RC.ERROR


    # *
    # * @param user - User name to disconnect from the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def disconnect(user, window):
        """Esta función se encarga de desconectar al usuario en el servidor.
            Recibe el nombre de usuario y la ventana de la interfaz gráfica.
            Creará un socket y se conectará al servidor, enviando el código de operación y la información necesaria."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            sock.connect((client._server, client._port))
            
            sock.sendall(b'DISCONNECT\0')
            sock.sendall(bytes(user + '\0', 'utf-8'))
        except :
            window['_SERVER_'].print("s> DISCONNECT FAIL")
        else:
            #Recibe la respuesta del servidor y, dependiendo del código de respuesta, muestra el mensaje por pantalla.
            response = int(sock.recv(1).decode('utf-8'))
            match response:
                case 0:
                    msgSocket.close()
                    window['_SERVER_'].print("s> DISCONNECT OK")
                    return
                case 1:
                    window['_SERVER_'].print("s> DISCONNECT FAIL / USER DOES NOT EXIST")
                    return
                case 2:
                    window['_SERVER_'].print("s> DISCONNECT FAIL / USER NOT CONNECTED")
                    return
                case 3:
                    window['_SERVER_'].print("s> DISCONNECT FAIL")
                    return
        finally:
            sock.close()

        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def send(user, message, window):
        """Esta función se encarga de enviar un mensaje a un usuario.
            Recibe el nombre de usuario, el mensaje y la ventana de la interfaz gráfica.
            Creará un socket y se conectará al servidor, enviando el código de operación y la información necesaria.
            Además, se encargará de quitar los espacios del mensaje y enviarlo al servidor."""
        
        # Quitamos los espacios del mensaje    
        soap = zeep.Client(wsdl = web_url)
        transf_msg = soap.service.quitar_espacios(message)

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            sock.connect((client._server, client._port))
            
            sock.sendall(b'SEND\0')
            sock.sendall(bytes(client._alias + '\0', 'utf-8'))
            sock.sendall(bytes(user + '\0', 'utf-8'))
            sock.sendall(bytes(transf_msg + '\0', 'utf-8'))
                        
        except:
            window['_SERVER_'].print("s> SEND FAIL")
            
        else:
            # Recibe la respuesta del servidor y, dependiendo del código de respuesta, muestra el mensaje por pantalla.
            response = int(sock.recv(1).decode('utf-8'))
            match response:
                case 0:
                    id = readMessage(sock)
                    window['_SERVER_'].print("s> SEND OK - MESSAGE " + id)
                    return
                case 1:
                    window['_SERVER_'].print("s> SEND FAIL / USER DOES NOT EXIST")
                    return
                case 2:
                    window['_SERVER_'].print("s> SEND FAIL")
                    return
        
        finally:
            sock.close()
        #  Write your code here
        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * @param file    - file  to be sent

    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def sendAttach(user, message, file, window):
        window['_SERVER_'].print("s> SENDATTACH MESSAGE OK")
        print("SEND ATTACH " + user + " " + message + " " + file)
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def connectedUsers(window):
        """Esta función se encarga de mostrar los usuarios conectados en el servidor.
            Recibe la ventana de la interfaz gráfica.
            Creará un socket y se conectará al servidor, enviando el código de operación y la información necesaria."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        try:
            sock.connect((client._server, client._port))
            
            sock.sendall(b'CONNECTEDUSERS\0')
            sock.sendall(bytes(client._alias + '\0', 'utf-8')) 
        except :
            window['_SERVER_'].print("s> CONNECTED USERS FAIL")
        else:
            # Recibe la respuesta del servidor y, dependiendo del código de respuesta, muestra el mensaje por pantalla.
            response = int(sock.recv(1).decode('utf-8'))
            match response:
                case 0:
                    users = ""
                    user_count = sock.recv(4)
                    user_count = int.from_bytes(user_count, byteorder='big')
                    users = readMessage(sock)
                    # Recibe el número de usuarios conectados e itera para recibir los nombres de los usuarios.
                    for i in range(1 ,user_count):
                        user = readMessage(sock)
                        users += ", " + user 
                    window['_SERVER_'].print("s> CONNECTED USERS ", user_count," OK - ", users)
                    return
                case 1:
                    window['_SERVER_'].print("s> CONNECTED USERS FAIL / USER IS NOT CONNECTED")                      
                    return
                case 2:
                    window['_SERVER_'].print("s> CONNECTED USERS FAIL")
                    return
        
        return client.RC.ERROR


    @staticmethod
    def window_register():
        layout_register = [[sg.Text('Ful Name:'),sg.Input('Text',key='_REGISTERNAME_', do_not_clear=True, expand_x=True)],
                            [sg.Text('Alias:'),sg.Input('Text',key='_REGISTERALIAS_', do_not_clear=True, expand_x=True)],
                            [sg.Text('Date of birth:'),sg.Input('',key='_REGISTERDATE_', do_not_clear=True, expand_x=True, disabled=True, use_readonly_for_disable=False),
                            sg.CalendarButton("Select Date",close_when_date_chosen=True, target="_REGISTERDATE_", format='%d/%m/%Y',size=(10,1))],
                            [sg.Button('SUBMIT', button_color=('white', 'blue'))]
                            ]

        layout = [[sg.Column(layout_register, element_justification='center', expand_x=True, expand_y=True)]]

        window = sg.Window("REGISTER USER", layout, modal=True)
        choice = None

        while True:
            event, values = window.read()

            if (event in (sg.WINDOW_CLOSED, "-ESCAPE-")):
                break

            if event == "SUBMIT":
                if(values['_REGISTERNAME_'] == 'Text' or values['_REGISTERNAME_'] == '' or values['_REGISTERALIAS_'] == 'Text' or values['_REGISTERALIAS_'] == '' or values['_REGISTERDATE_'] == ''):
                    sg.Popup('Registration error', title='Please fill in the fields to register.', button_type=5, auto_close=True, auto_close_duration=1)
                    continue

                client._username = values['_REGISTERNAME_']
                client._alias = values['_REGISTERALIAS_']
                client._date = values['_REGISTERDATE_']
                break
        window.Close()


    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")
            return False

        client._server = args.s
        client._port = args.p

        return True

    def main(argv):

        if (not client.parseArguments(argv)):
            client.usage()
            exit()

        lay_col = [[sg.Button('REGISTER',expand_x=True, expand_y=True),
                sg.Button('UNREGISTER',expand_x=True, expand_y=True),
                sg.Button('CONNECT',expand_x=True, expand_y=True),
                sg.Button('DISCONNECT',expand_x=True, expand_y=True),
                sg.Button('CONNECTED USERS',expand_x=True, expand_y=True)],
                [sg.Text('Dest:'),sg.Input('User',key='_INDEST_', do_not_clear=True, expand_x=True),
                sg.Text('Message:'),sg.Input('Text',key='_IN_', do_not_clear=True, expand_x=True),
                sg.Button('SEND',expand_x=True, expand_y=False)],
                [sg.Text('Attached File:'), sg.In(key='_FILE_', do_not_clear=True, expand_x=True), sg.FileBrowse(),
                sg.Button('SENDATTACH',expand_x=True, expand_y=False)],
                [sg.Multiline(key='_CLIENT_', disabled=True, autoscroll=True, size=(60,15), expand_x=True, expand_y=True),
                sg.Multiline(key='_SERVER_', disabled=True, autoscroll=True, size=(60,15), expand_x=True, expand_y=True)],
                [sg.Button('QUIT', button_color=('white', 'red'))]
            ]


        layout = [[sg.Column(lay_col, element_justification='center', expand_x=True, expand_y=True)]]

        window = sg.Window('Messenger', layout, resizable=True, finalize=True, size=(1000,400))
        window.bind("<Escape>", "-ESCAPE-")


        while True:
            event, values = window.Read()

            if (event in (None, 'QUIT')) or (event in (sg.WINDOW_CLOSED, "-ESCAPE-")):
                sg.Popup('Closing Client APP', title='Closing', button_type=5, auto_close=True, auto_close_duration=1)
                break

            #if (values['_IN_'] == '') and (event != 'REGISTER' and event != 'CONNECTED USERS'):
             #   window['_CLIENT_'].print("c> No text inserted")
             #   continue

            if (client._alias == None or client._username == None or client._alias == 'Text' or client._username == 'Text' or client._date == None) and (event != 'REGISTER'):
                sg.Popup('NOT REGISTERED', title='ERROR', button_type=5, auto_close=True, auto_close_duration=1)
                continue

            if (event == 'REGISTER'):
                client.window_register()

                if (client._alias == None or client._username == None or client._alias == 'Text' or client._username == 'Text' or client._date == None):
                    sg.Popup('NOT REGISTERED', title='ERROR', button_type=5, auto_close=True, auto_close_duration=1)
                    continue

                window['_CLIENT_'].print('c> REGISTER ' + client._alias)
                client.register(client._alias, window)

            elif (event == 'UNREGISTER'):
                window['_CLIENT_'].print('c> UNREGISTER ' + client._alias)
                client.unregister(client._alias, window)


            elif (event == 'CONNECT'):
                window['_CLIENT_'].print('c> CONNECT ' + client._alias)
                client.connect(client._alias, window)


            elif (event == 'DISCONNECT'):
                window['_CLIENT_'].print('c> DISCONNECT ' + client._alias)
                client.disconnect(client._alias, window)


            elif (event == 'SEND'):
                window['_CLIENT_'].print('c> SEND ' + values['_INDEST_'] + " " + values['_IN_'])

                if (values['_INDEST_'] != '' and values['_IN_'] != '' and values['_INDEST_'] != 'User' and values['_IN_'] != 'Text') :
                    client.send(values['_INDEST_'], values['_IN_'], window)
                else :
                    window['_CLIENT_'].print("Syntax error. Insert <destUser> <message>")


            elif (event == 'SENDATTACH'):

                window['_CLIENT_'].print('c> SENDATTACH ' + values['_INDEST_'] + " " + values['_IN_'] + " " + values['_FILE_'])

                if (values['_INDEST_'] != '' and values['_IN_'] != '' and values['_FILE_'] != '') :
                    client.sendAttach(values['_INDEST_'], values['_IN_'], values['_FILE_'], window)
                else :
                    window['_CLIENT_'].print("Syntax error. Insert <destUser> <message> <attachedFile>")


            elif (event == 'CONNECTED USERS'):
                window['_CLIENT_'].print("c> CONNECTEDUSERS")
                client.connectedUsers(window)



            window.Refresh()

        window.Close()


if __name__ == '__main__':
    client.main([])
    print("+++ FINISHED +++")
