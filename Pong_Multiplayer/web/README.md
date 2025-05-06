# Pasta `web/` - Servidor Flask para controle via HTTP

Esta pasta contém o código do **servidor web Flask** que recebe comandos enviados pela placa **Raspberry Pi Pico W** via requisições HTTP. O servidor interpreta comandos como `/left` e `/right` por meio do SocketIO, que podem ser usados para controlar um jogo ou aplicação no navegador.

## 📂 Estrutura esperada

O conteúdo típico da pasta deve incluir um servidor Flask simples e arquivos HTML/JS, como:

- `server.py` — Arquivo principal que inicializa o servidor Flask.
- `static/` — Pasta com arquivos JavaScript, CSS e imagens.
- `templates/` — Pasta com arquivos HTML.

## 🚀 Como executar o servidor

1. Instale o flask:
   ```bash
   pip install flask
   ```
2. Instale o flask-socketio:
   ```bash
   pip install flask-socketio
   ```

3. Execute o servidor:
   ```bash
   python server.py
   ```

4. Acesse no navegador:
   ```
   http://localhost:5000
   ```

> Certifique-se de que a Pico W esteja configurada para enviar requisições para o endereço IP da máquina onde o Flask está rodando.

## 🔁 Endpoints esperados

O cliente Pico W envia requisições GET para os seguintes endpoints:

- `/left` — Comando de controle para a esquerda.
- `/right` — Comando de controle para a direita.

Você pode adaptar o backend para interpretar esses comandos como quiser (por exemplo, controlar um personagem em um jogo ou movimentar algo na tela).

## 💡 Exemplo de uso

A aplicação pode ser integrada com um jogo HTML/JS que responde aos comandos recebidos via WebSocket ou outra comunicação backend.

---

> 💬 Dica: para testes rápidos, você pode simular os comandos no navegador acessando diretamente `http://localhost:5000/left` e `http://localhost:5000/right`.

---

> **Se você encontrar dificuldade para configurar o servidor veja um tutorial completo de como configurar corretamente na rp2040:** https://github.com/davissontiago/Tutorial_Servidor_Flask_Pico/blob/main/README.md