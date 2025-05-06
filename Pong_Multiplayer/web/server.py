from flask import Flask, render_template, request
from flask_socketio import SocketIO, emit
from threading import Lock
import logging

# Configuração básica de logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'

# Configuração do Socket.IO com a versão 5.3.6
socketio = SocketIO(app, 
                   cors_allowed_origins="*",
                   logger=True,
                   engineio_logger=True,
                   async_mode='threading')

# Lock para thread safety
thread_lock = Lock()

@app.route('/')
def index():
    """Rota principal que serve a página do jogo"""
    return render_template('index.html')

@app.route('/control', methods=['GET'])
def handle_control():
    try:
        with thread_lock:
            player = int(request.args.get('player', 1))
            action = request.args.get('action', '').lower()
            value_str = request.args.get('value', '0')
            
            # Permite valores negativos convertendo corretamente
            value = int(value_str) if value_str.lstrip('-').isdigit() else 0
            
            logger.info(f"Recebido - Player: {player}, Action: {action}, Value: {value}")
            
            if action == 'move' and player in (1, 2):
                socketio.emit('player_move', {
                    'player': player,
                    'value': value  # Agora aceita positivos e negativos
                }, namespace='/', to=None)
                
                return 'OK', 200
            
            logger.warning(f"Comando inválido - Player: {player}, Action: {action}")
            return 'Comando inválido', 400
            
    except ValueError as e:
        logger.error(f"Erro de valor: {str(e)}")
        return f'Erro de valor: {str(e)}', 400
    except Exception as e:
        logger.error(f"Erro interno: {str(e)}", exc_info=True)
        return 'Erro no servidor', 500

@socketio.on('connect')
def handle_connect():
    """Evento quando um cliente se conecta via WebSocket"""
    logger.info(f'Cliente WebSocket conectado: {request.sid}')
    emit('connection_response', {'status': 'connected'})

@socketio.on('disconnect')
def handle_disconnect():
    """Evento quando um cliente desconecta"""
    logger.info(f'Cliente WebSocket desconectado: {request.sid}')

if __name__ == '__main__':
    logger.info("Iniciando servidor PongMultiplayer...")
    socketio.run(app, 
                host='0.0.0.0', 
                port=5000, 
                debug=True, 
                allow_unsafe_werkzeug=True)