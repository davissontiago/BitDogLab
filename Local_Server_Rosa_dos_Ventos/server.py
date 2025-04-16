from flask import Flask, render_template, request
from flask_socketio import SocketIO, emit

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, cors_allowed_origins="*")  # Permite todas as origens

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/mensagem')
def receber_mensagem():
    msg = request.args.get('msg')
    if msg:
        print("Mensagem recebida:", msg)
        # Modificação aqui - removido o broadcast=True
        socketio.emit('nova_direcao', msg, namespace='/')
    return "OK"

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)