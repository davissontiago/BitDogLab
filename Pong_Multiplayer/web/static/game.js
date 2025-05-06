// Configurações do jogo
const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
const socket = io();

// Constantes do jogo
const PADDLE_WIDTH = 10;
const PADDLE_HEIGHT = 100;
const BALL_SIZE = 20;
const INITIAL_BALL_SPEED = 8;  
const BALL_ACCELERATION = 1.05; 
const PLAYER_COLORS = ['#FF6666', '#6666FF'];
const PADDLE_ACCELERATION = 0.4;
const PADDLE_FRICTION = 0.8;
const PADDLE_MAX_SPEED = 15;

// Configurações da arena
const ARENA_PADDING = 30;
const ARENA_WIDTH = canvas.width - ARENA_PADDING * 2;
const ARENA_HEIGHT = canvas.height - ARENA_PADDING * 2;
const BORDER_WIDTH = 2;
const MAX_SCORE = 7;
const GAME_OVER_DELAY = 3000;

// Variáveis globais
let gamePaused = false;
let lastScorer = null;
let lastTime = 0;
const frameDelay = 1000 / 60; // 60 FPS

const pointMessage = document.createElement('div');
pointMessage.className = 'point-message';
document.body.appendChild(pointMessage);

// Estado do jogo
const players = [
    { y: ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2, score: 0, velocity: 0 },
    { y: ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2, score: 0, velocity: 0 }
];

let ball = {
    x: ARENA_WIDTH / 2 - BALL_SIZE / 2,
    y: ARENA_HEIGHT / 2 - BALL_SIZE / 2,
    speedX: INITIAL_BALL_SPEED * (Math.random() > 0.5 ? 1 : -1),
    speedY: (Math.random() - 0.5) * INITIAL_BALL_SPEED
};

// Função de desenho
function draw() {
    // Fundo
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // Arena
    ctx.fillStyle = '#111';
    ctx.fillRect(ARENA_PADDING, ARENA_PADDING, ARENA_WIDTH, ARENA_HEIGHT);

    // Borda da arena
    ctx.strokeStyle = '#444';
    ctx.lineWidth = BORDER_WIDTH;
    ctx.strokeRect(ARENA_PADDING, ARENA_PADDING, ARENA_WIDTH, ARENA_HEIGHT);

    // Linha central
    ctx.strokeStyle = '#333';
    ctx.setLineDash([15, 10]);
    ctx.beginPath();
    ctx.moveTo(canvas.width / 2, ARENA_PADDING);
    ctx.lineTo(canvas.width / 2, canvas.height - ARENA_PADDING);
    ctx.stroke();
    ctx.setLineDash([]);

    // Players
    players.forEach((player, index) => {
        const x = index === 0 ? ARENA_PADDING : canvas.width - ARENA_PADDING - PADDLE_WIDTH;
        const gradient = ctx.createLinearGradient(x, 0, x + PADDLE_WIDTH, 0);
        gradient.addColorStop(0, PLAYER_COLORS[index]);
        gradient.addColorStop(1, index === 0 ? '#330000' : '#000033');
        ctx.fillStyle = gradient;
        ctx.fillRect(x, ARENA_PADDING + player.y, PADDLE_WIDTH, PADDLE_HEIGHT);
    });

    // Bola
    const ballGradient = ctx.createRadialGradient(
        ARENA_PADDING + ball.x + BALL_SIZE / 2,
        ARENA_PADDING + ball.y + BALL_SIZE / 2,
        0,
        ARENA_PADDING + ball.x + BALL_SIZE / 2,
        ARENA_PADDING + ball.y + BALL_SIZE / 2,
        BALL_SIZE / 2
    );
    ballGradient.addColorStop(0, '#FFFFFF');
    ballGradient.addColorStop(1, '#AAAAAA');
    ctx.fillStyle = ballGradient;
    ctx.beginPath();
    ctx.arc(
        ARENA_PADDING + ball.x + BALL_SIZE / 2,
        ARENA_PADDING + ball.y + BALL_SIZE / 2,
        BALL_SIZE / 2,
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

// Atualização do jogo
function update() {
    if (gamePaused) return;

    // Movimento da bola
    ball.x += ball.speedX;
    ball.y += ball.speedY;

    // Colisão com bordas
    if (ball.y <= 0) {
        ball.y = 0;
        ball.speedY = -ball.speedY;
    } else if (ball.y + BALL_SIZE >= ARENA_HEIGHT) {
        ball.y = ARENA_HEIGHT - BALL_SIZE;
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
            
            // Aumenta a velocidade gradualmente
            const currentSpeed = Math.sqrt(ball.speedX**2 + ball.speedY**2);
            const newSpeed = currentSpeed * BALL_ACCELERATION;
            
            ball.speedX = newSpeed * Math.cos(angle) * (index === 0 ? 1 : -1);
            ball.speedY = newSpeed * Math.sin(angle);
        }
    });

    // Pontuação
    if (ball.x < 0) scorePoint(1);
    else if (ball.x > ARENA_WIDTH) scorePoint(0);

    // Atualiza players
    players.forEach(player => {
        player.velocity *= PADDLE_FRICTION;
        if (Math.abs(player.velocity) > PADDLE_MAX_SPEED) {
            player.velocity = PADDLE_MAX_SPEED * Math.sign(player.velocity);
        }
        player.y += player.velocity;
        player.y = Math.max(0, Math.min(ARENA_HEIGHT - PADDLE_HEIGHT, player.y));
    });
}

function scorePoint(playerIndex) {
    players[playerIndex].score++;
    lastScorer = playerIndex;

    // Mostra mensagem de ponto
    showPointMessage(playerIndex);

    // Verifica se o jogo acabou
    if (players[playerIndex].score >= MAX_SCORE) {
        gameOver(playerIndex);
        return;
    }

    // Pausa o jogo por 1 segundo
    gamePaused = true;
    setTimeout(() => {
        gamePaused = false;
        pointMessage.classList.remove('visible');
        resetBall(playerIndex === 0 ? -1 : 1);
    }, 1000);
}

function gameOver(winnerIndex) {
    gamePaused = true;

    // Primeiro remove todas as classes de jogador
    pointMessage.classList.remove('player1', 'player2');
    
    // Adiciona a classe correta
    pointMessage.classList.add(`player${winnerIndex + 1}`);
    
    // Define o conteúdo e mostra
    pointMessage.textContent = `JOGADOR ${winnerIndex + 1} VENCEU!`;
    pointMessage.classList.add('visible');

    // Reinicia o jogo após o delay
    setTimeout(() => {
        resetGame();
        pointMessage.classList.remove('visible');
        gamePaused = false;
    }, GAME_OVER_DELAY);
}

function resetGame() {
    // Reinicia placar
    players[0].score = 0;
    players[1].score = 0;

    // Reinicia posições
    players[0].y = ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    players[1].y = ARENA_HEIGHT / 2 - PADDLE_HEIGHT / 2;

    // Reinicia bola
    resetBall(Math.random() > 0.5 ? 1 : -1);

    // Reinicia velocidades
    players[0].velocity = 0;
    players[1].velocity = 0;
}

function showPointMessage(playerIndex) {
    // Primeiro remove todas as classes de jogador
    pointMessage.classList.remove('player1', 'player2');
    
    // Adiciona a classe correta
    pointMessage.classList.add(`player${playerIndex + 1}`);
    
    // Define o conteúdo
    if (players[playerIndex].score >= MAX_SCORE) {
        pointMessage.textContent = `JOGADOR ${playerIndex + 1} VENCEU!`;
    } else {
        pointMessage.textContent = 'PONTO!';
    }
    
    // Mostra a mensagem
    pointMessage.classList.add('visible');
}

function resetBall(direction) {
    ball = {
        x: ARENA_WIDTH / 2 - BALL_SIZE / 2,
        y: ARENA_HEIGHT / 2 - BALL_SIZE / 2,
        speedX: INITIAL_BALL_SPEED * direction,
        speedY: (Math.random() - 0.5) * INITIAL_BALL_SPEED
    };
}

// Controles
document.addEventListener('keydown', (e) => {
    if (e.key === 'w') players[0].velocity -= PADDLE_ACCELERATION;
    if (e.key === 's') players[0].velocity += PADDLE_ACCELERATION;
    if (e.key === 'ArrowUp') players[1].velocity -= PADDLE_ACCELERATION;
    if (e.key === 'ArrowDown') players[1].velocity += PADDLE_ACCELERATION;
});

socket.on('player_move', (data) => {
    if (data.player === 1 || data.player === 2) {
        const playerIndex = data.player - 1;
        players[playerIndex].velocity += data.value * PADDLE_ACCELERATION;
    }
});

let lastRenderTime = 0;
const FPS = 60;

// Game loop otimizado
function gameLoop(timestamp) {
    // Controle de FPS para animação suave
    const secondsSinceLastRender = (timestamp - lastRenderTime) / 1000;
    if (secondsSinceLastRender < 1 / FPS) {
        requestAnimationFrame(gameLoop);
        return;
    }
    lastRenderTime = timestamp;
    
    // Limpa o canvas de forma mais eficiente
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    update();
    draw();
    
    requestAnimationFrame(gameLoop);
}

// Iniciar jogo
gameLoop();