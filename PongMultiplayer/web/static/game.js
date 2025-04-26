// Configurações do jogo
const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
const socket = io();

const PADDLE_WIDTH = 10;
const PADDLE_HEIGHT = 100;
const BALL_SIZE = 25;
const PLAYER_COLORS = ['#FF6666', '#6666FF'];

// Configurações da arena
const ARENA_PADDING = 30;
const ARENA_WIDTH = canvas.width - ARENA_PADDING * 2;
const ARENA_HEIGHT = canvas.height - ARENA_PADDING * 2;
const BORDER_WIDTH = 2;

// Variáveis globais adicionais
let gamePaused = false;
let lastScorer = null;
const pointMessage = document.createElement('div');
pointMessage.className = 'point-message';
document.body.appendChild(pointMessage);

// Estado do jogo
const players = [
    { y: ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2, score: 0, velocity: 0 }, // Player 1
    { y: ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2, score: 0, velocity: 0 }  // Player 2
];

let ball = {
    x: ARENA_WIDTH / 2 - BALL_SIZE / 2,
    y: ARENA_HEIGHT / 2 - BALL_SIZE / 2,
    speedX: 5,
    speedY: 5
};

// Desenhar elementos
function draw() {
    // Fundo
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // Arena com borda
    ctx.fillStyle = '#111';
    ctx.fillRect(ARENA_PADDING, ARENA_PADDING, ARENA_WIDTH, ARENA_HEIGHT);
    
    // Borda da arena
    ctx.strokeStyle = '#444';
    ctx.lineWidth = BORDER_WIDTH;
    ctx.strokeRect(ARENA_PADDING, ARENA_PADDING, ARENA_WIDTH, ARENA_HEIGHT);
    
    // Linha central tracejada
    ctx.strokeStyle = '#333';
    ctx.setLineDash([15, 10]);
    ctx.beginPath();
    ctx.moveTo(canvas.width / 2, ARENA_PADDING);
    ctx.lineTo(canvas.width / 2, canvas.height - ARENA_PADDING);
    ctx.stroke();
    ctx.setLineDash([]);
    
    // Players
    players.forEach((player, index) => {
        ctx.fillStyle = PLAYER_COLORS[index];
        const x = index === 0 ? ARENA_PADDING : canvas.width - ARENA_PADDING - PADDLE_WIDTH;
        ctx.fillRect(x, ARENA_PADDING + player.y, PADDLE_WIDTH, PADDLE_HEIGHT);
        
        // Efeito de luz nos paddles
        const gradient = ctx.createLinearGradient(x, 0, x + PADDLE_WIDTH, 0);
        gradient.addColorStop(0, index === 0 ? '#FF6666' : '#6666FF');  // Cores principais
        gradient.addColorStop(1, index === 0 ? '#330000' : '#000033'); 
        ctx.fillStyle = gradient;
        ctx.fillRect(x, ARENA_PADDING + player.y, PADDLE_WIDTH, PADDLE_HEIGHT);
    });
    
    // Bola
    const ballGradient = ctx.createRadialGradient(
        ARENA_PADDING + ball.x + BALL_SIZE/2, 
        ARENA_PADDING + ball.y + BALL_SIZE/2, 
        0,
        ARENA_PADDING + ball.x + BALL_SIZE/2, 
        ARENA_PADDING + ball.y + BALL_SIZE/2, 
        BALL_SIZE/2
    );
    ballGradient.addColorStop(0, '#FFFFFF');
    ballGradient.addColorStop(1, '#AAAAAA');
    ctx.fillStyle = ballGradient;
    ctx.beginPath();
    ctx.arc(
        ARENA_PADDING + ball.x + BALL_SIZE/2, 
        ARENA_PADDING + ball.y + BALL_SIZE/2, 
        BALL_SIZE/2, 
        0, 
        Math.PI * 2
    );
    ctx.fill();
    
    // Placar
    ctx.fillStyle = '#FFF';
    ctx.font = 'bold 32px Arial';
    ctx.textAlign = 'center';
    ctx.fillText(players[0].score, canvas.width / 4, ARENA_PADDING - 4);
    ctx.fillText(players[1].score, (canvas.width / 4) * 3, ARENA_PADDING - 4);
}

// Atualizar lógica do jogo
function update() {
    if (gamePaused) return; // Pausa o jogo
    
    // Mover bola
    ball.x += ball.speedX;
    ball.y += ball.speedY;
    
    // Colisão com topo/fundo da arena
    if (ball.y <= 0 || ball.y + BALL_SIZE >= ARENA_HEIGHT) {
        ball.speedY = -ball.speedY;
    }
    
    // Colisão com players
    players.forEach((player, index) => {
        const playerX = index === 0 ? 0 : ARENA_WIDTH - PADDLE_WIDTH;
        
        if (ball.x <= playerX + PADDLE_WIDTH && 
            ball.x + BALL_SIZE >= playerX && 
            ball.y + BALL_SIZE >= player.y && 
            ball.y <= player.y + PADDLE_HEIGHT) {
            
            const hitPos = (ball.y - player.y) / PADDLE_HEIGHT;
            const angle = (hitPos - 0.5) * Math.PI / 2;
            
            const speed = Math.sqrt(ball.speedX**2 + ball.speedY**2) * 1.10;
            ball.speedX = speed * Math.cos(angle) * (index === 0 ? 1 : -1);
            ball.speedY = speed * Math.sin(angle);
        }
    });
    
    // Pontuação e reset da bola
    if (ball.x < 0) {
        scorePoint(1); // Player 2 fez ponto
    } else if (ball.x > ARENA_WIDTH) {
        scorePoint(0); // Player 1 fez ponto
    }
    
    // Atualizar posição dos players
    players.forEach(player => {
        player.y += player.velocity;
        player.y = Math.max(0, Math.min(ARENA_HEIGHT - PADDLE_HEIGHT, player.y));
        player.velocity *= 0.3;
    });
}

function scorePoint(playerIndex) {
    players[playerIndex].score++;
    lastScorer = playerIndex;
    
    // Mostra mensagem de ponto
    showPointMessage(playerIndex);
    
    // Pausa o jogo por 1 segundo
    gamePaused = true;
    setTimeout(() => {
        gamePaused = false;
        pointMessage.classList.remove('visible');
        resetBall(playerIndex === 0 ? -1 : 1);
    }, 1000);
}

function showPointMessage(playerIndex) {
    pointMessage.textContent = 'PONTO!';
    pointMessage.style.color = PLAYER_COLORS[playerIndex];
    pointMessage.classList.add('visible');
}

function resetBall(direction) {
    ball = {
        x: ARENA_WIDTH / 2 - BALL_SIZE / 2,
        y: ARENA_HEIGHT / 2 - BALL_SIZE / 2,
        speedX: 5 * direction,
        speedY: (Math.random() - 0.5) * 10
    };
}

// Controle por Socket.IO
socket.on('player_move', (data) => {
    if (data.player === 1 || data.player === 2) {
        const playerIndex = data.player - 1;
        players[playerIndex].velocity = data.value * 0.3;
    }
});

// Controle por teclado (para testes)
document.addEventListener('keydown', (e) => {
    const speed = 15;
    if (e.key === 'w') players[0].velocity = -speed;
    if (e.key === 's') players[0].velocity = speed;
    if (e.key === 'ArrowUp') players[1].velocity = -speed;
    if (e.key === 'ArrowDown') players[1].velocity = speed;
});

// Loop do jogo
function gameLoop() {
    draw();
    update();
    requestAnimationFrame(gameLoop);
}

// Iniciar o jogo
gameLoop();