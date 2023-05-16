

from spyne import Application, ServiceBase, Unicode, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication


class QuitarEspacios(ServiceBase):

    @rpc(Unicode, _returns=Unicode)
    def quitar_espacios(ctx, texto):
        #reducir todas las secuencias de espacios a un solo espacio
        texto_res = ""
        for i in range(len(texto)):
            if i == 0 and texto[i] == " ":
                continue
            if texto[i] == " " and texto[i-1] == " ":
                continue
            else:
                texto_res += texto[i]
        return texto_res
    
application = Application(
    services=[QuitarEspacios],
    tns='quitar_espacios',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11())

application = WsgiApplication(application)

if __name__ == '__main__':
    import logging

    from wsgiref.simple_server import make_server

    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger('spyne.protocol.xml').setLevel(logging.DEBUG)

    logging.info("listening to http://127.0.0.1:8000")
    logging.info("wsdl is at: http://localhost:8000/?wsdl")

    server = make_server('127.0.0.1', 8000, application)
    server.serve_forever()

