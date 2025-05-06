#include <stdio.h>
#include "menu.h"
#include "snakeGame.c"

void show_menu() {
    int opcao;

    while (1) {
        printf("=== MENU ===\n");
        printf("1. Jogar Snake\n");
        printf("2. Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);

        if (opcao == 1) {
            start_snake_game(); // Função que você já tenha em snakeGame.c
        } else if (opcao == 2) {
            printf("Saindo...\n");
            break;
        } else {
            printf("Opção inválida!\n");
        }
    }
}
