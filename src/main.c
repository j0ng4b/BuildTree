// Includes
//
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"


// Defines
//
#define GAME_WINDOW_TITLE "BuildTree"

#define GAME_WINDOW_WIDTH 600
#define GAME_WINDOW_HALF_WIDTH (GAME_WINDOW_WIDTH / 2.0)

#define GAME_WINDOW_HEIGHT 480
#define GAME_WINDOW_HALF_HEIGHT (GAME_WINDOW_HEIGHT / 2.0)

#define GAME_MIN_VALUE 50
#define GAME_MAX_VALUE 150

#define GAME_FONT_SIZE 35
#define GAME_FONT_SIZE_HALF (GAME_FONT_SIZE / 2.0)

#define GAME_POPUP_MAX_TITLE_SIZE 25
#define GAME_POPUP_MAX_MESSAGE_SIZE 150
#define GAME_POPUP_MAX_ACTION_TEXT_SIZE 10

#define GAME_POPUP_TITLE_FONT_SIZE 20
#define GAME_POPUP_MESSAGE_FONT_SIZE 17
#define GAME_POPUP_ACTION_TEXT_FONT_SIZE 20

// Macros
#define RANDINT(min, max) ((min) + rand() % ((max) - (min)))


// Enumerations
//
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_IN_GAME,
} GameState;

// Structures
//
typedef struct node {
    int value;

    struct node *left;
    struct node *right;
} Node;

typedef struct {
    Node *treeRoot;
    Node *currentTreeNode;

    struct {
        double current;
        double remain;
        double start;
    } time;

    // Store the current and previously raffled values
    struct {
        int current;
        int count;

        int *generated;
    } values;

    struct {
        bool showing;

        char title[GAME_POPUP_MAX_TITLE_SIZE];
        char message[GAME_POPUP_MAX_MESSAGE_SIZE];

        Rectangle bounds;

        struct {
            char text[GAME_POPUP_MAX_TITLE_SIZE];

            bool hovering;
            bool down;

            Rectangle bounds;

            void (*callback)(void *);
        } actionLeft, actionRight;
    } popup;

    GameState state;
} Game;


// Function prototypes
//
void gameMenu(Game *game);

Node *createNode(int value);
void drawNode(Node *node, int posX, int posY, Color colors[3]);

void runInGame(Game *game);

void createGamePopup(Game *game, char title[], char message[]);
void updateGamePopup(Game *game);
void drawGamePopup(Game *game);

void drawCurrentNumber(Game *game, Color color);
void drawRemainTime(Game *game, Color colors[2]);

void resetGame(Game *game);
void generateNewValue(Game *game);
void destroyTree(Node *node);

// Function implementation
//

void quitGame(void *game)
{
    exit(EXIT_SUCCESS);
}

void startGame(void *game)
{
    ((Game *) game)->popup.showing = false;
    ((Game *) game)->state = GAME_STATE_IN_GAME;
}

int main (int argc, char *argv[])
{
    Game game = {
        .treeRoot = NULL,

        .values = {
            .count = 0,
            .generated = NULL,
        },

        .state = GAME_STATE_MENU,
    };

    srand(time(NULL));

    InitWindow(GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT, GAME_WINDOW_TITLE);
    SetTargetFPS(60);

    createGamePopup(&game, "Iniciar jogo?", "\n Para jogar use as setas\n para escolher o lado que o\n número deve ser inserido.");

    strcpy(game.popup.actionLeft.text, "Sair");
    game.popup.actionLeft.callback = quitGame;

    strcpy(game.popup.actionRight.text, "Jogar");
    game.popup.actionRight.callback = startGame;

    while (!WindowShouldClose()) {
        switch (game.state) {
            case GAME_STATE_MENU:
                gameMenu(&game);
                break;

            case GAME_STATE_IN_GAME:
                if (game.treeRoot == NULL) {
                    resetGame(&game);
                    WaitTime(800);
                }
                runInGame(&game);
                break;
        }
    }

    CloseWindow();
    return EXIT_SUCCESS;
}


void gameMenu(Game *game)
{

    Texture2D background = LoadTexture("assets/background3.png");
    Texture2D arvore = LoadTexture("assets/arvore.png");
    Texture2D titulo = LoadTexture("assets/titulo.png");
    ClearBackground(WHITE);
    DrawTexture(background,1,1,WHITE);
    DrawTexture(arvore,400,290 , WHITE);
    DrawTexture(arvore,10,290 , WHITE);
    DrawTexture(titulo,100,10,GOLD);


    BeginDrawing();

    if (game->popup.showing) {
        updateGamePopup(game);
        drawGamePopup(game);
    }

    EndDrawing();


}

Node *createNode(int value)
{
    Node *newNode = (Node *) calloc(1, sizeof(Node));

    newNode->value = value;

    newNode->right = NULL;
    newNode->left = NULL;

    return newNode;
}

void drawNode(Node *node, int posX, int posY, Color colors[3])
{
    char nodeValue[15];
    int nodeValueWidth;

    sprintf(nodeValue, "%d", node->value);
    nodeValueWidth = MeasureText(nodeValue, GAME_FONT_SIZE);

    DrawCircle(posX, posY, GAME_FONT_SIZE, colors[0]);
    DrawCircle(posX, posY, GAME_FONT_SIZE - 2, colors[1]);

    DrawText(nodeValue,
        posX - (nodeValueWidth / 2),
        posY - GAME_FONT_SIZE_HALF,
        GAME_FONT_SIZE,
        colors[2]);
}


void backToMenu(void *data)
{
    exit(1);
    ((Game *) data)->popup.showing = false;
    ((Game *) data)->state = GAME_STATE_MENU;

    createGamePopup(((Game *) data), "Iniciar jogo?", "\n Para jogar use as setas\n para escolher o lado que o\n número deve ser inserido.");

    strcpy(((Game *) data)->popup.actionLeft.text, "Sair");
    ((Game *) data)->popup.actionLeft.callback = quitGame;

    strcpy(((Game *) data)->popup.actionRight.text, "Jogar");
    ((Game *) data)->popup.actionRight.callback = startGame;
}

void updateInGame(Game *game)
{
    static bool leftDown = false;
    static bool rightDown = false;

    char message[GAME_POPUP_MAX_MESSAGE_SIZE];

    if (IsKeyDown(KEY_LEFT))
        leftDown = true;

    if (IsKeyDown(KEY_RIGHT))
        rightDown = true;

    if (IsKeyUp(KEY_LEFT) && leftDown) {
        if (game->values.current > game->currentTreeNode->value) {
            sprintf(message, " Lado errado...\n\nSeu tempo: %.3lfs",
                (GetTime() - game->time.start));

            createGamePopup(game, "Gameover!!", message);

            strcpy(game->popup.actionLeft.text, "Sair");
            game->popup.actionLeft.callback = backToMenu;
        } else if (game->currentTreeNode->left != NULL) {
            game->currentTreeNode = game->currentTreeNode->left;
        } else {
            game->currentTreeNode->left = createNode(game->values.current);
            game->currentTreeNode = game->treeRoot;

            generateNewValue(game);
        }

        game->time.remain += 0.5;
        leftDown = false;
    }

    if (IsKeyUp(KEY_RIGHT) && rightDown) {
        if (game->values.current < game->currentTreeNode->value) {
            sprintf(message, " Lado errado...\n\nSeu tempo: %.3lfs",
                (GetTime() - game->time.start));

            createGamePopup(game, "Gameover!!", message);

            strcpy(game->popup.actionLeft.text, "Sair");
            game->popup.actionLeft.callback = backToMenu;
        } else if (game->currentTreeNode->right != NULL) {
            game->currentTreeNode = game->currentTreeNode->right;
        } else {
            game->currentTreeNode->right = createNode(game->values.current);
            game->currentTreeNode = game->treeRoot;

            generateNewValue(game);
        }

        game->time.remain += 0.5;
        rightDown = false;
    }

    game->time.remain = game->time.remain - (GetTime() - game->time.current);
    game->time.current = GetTime();

    if (game->time.remain <= 0) {
        game->time.remain = 0;

        sprintf(message, "Game timeout...\n\nSeu tempo: %.3lfs",
            (GetTime() - game->time.start));

        createGamePopup(game, "Gameover!!", message);

        strcpy(game->popup.actionLeft.text, "Sair");
        game->popup.actionLeft.callback = backToMenu;
    }
}

void drawInGame(Game *game)
{
    int nodePosX = GAME_WINDOW_HALF_WIDTH;
    int nodePosY = GAME_WINDOW_HALF_HEIGHT;

    if (game->currentTreeNode->left != NULL) {
        int leftNodePosX = nodePosX - nodePosY / 2;
        int leftNodePosY = nodePosY + 50;

        DrawLine(nodePosX, nodePosY, leftNodePosX, leftNodePosY, WHITE);
        drawNode(game->currentTreeNode->left,
            leftNodePosX,
            leftNodePosY,
            (Color []) {GRAY, RED, GRAY});
    }

    if (game->currentTreeNode->right != NULL) {
        int rightNodePosX = nodePosX + nodePosY / 2;
        int rightNodePosY = nodePosY + 50;

        DrawLine(nodePosX, nodePosY, rightNodePosX, rightNodePosY, WHITE);
        drawNode(game->currentTreeNode->right,
            rightNodePosX,
            rightNodePosY,
            (Color []) {GRAY, RED, GRAY});
    }

    drawNode(game->currentTreeNode, nodePosX, nodePosY, (Color []) {WHITE, RED, WHITE});

    drawCurrentNumber(game, WHITE);
    drawRemainTime(game, (Color []) {WHITE, RED});
}

void runInGame(Game *game)
{
    ClearBackground(BLACK);
    BeginDrawing();

    drawInGame(game);
    if (game->popup.showing)
        drawGamePopup(game);

    EndDrawing();

    if (game->popup.showing)
        updateGamePopup(game);
    else
        updateInGame(game);

}

void createGamePopup(Game *game, char title[], char message[])
{
#define MAX_WIDTH (GAME_WINDOW_WIDTH * 0.8)
    int width = 0;
    int height = 0;

    int measuredWidth;

    memset(&game->popup, 0, sizeof game->popup);

    game->popup.showing = true;

    strncpy(game->popup.title, title, GAME_POPUP_MAX_TITLE_SIZE);
    strncpy(game->popup.message, message, GAME_POPUP_MAX_MESSAGE_SIZE);

    for (int start = 0, lastSpace = 0, i = 0; i < strlen(message); i++) {
        if (message[i] == ' ') {
            game->popup.message[i] = '\0';
            measuredWidth = MeasureText(game->popup.message + start,
                GAME_POPUP_MESSAGE_FONT_SIZE);

            if (measuredWidth >= MAX_WIDTH) {
                start = lastSpace + 1;
                game->popup.message[lastSpace] = '\n';

                width = width < measuredWidth ? measuredWidth : width;
                height += GAME_POPUP_MESSAGE_FONT_SIZE;
            } else {
                game->popup.message[i] = ' ';
                lastSpace = i;
            }
        } else if (message[i] == '\n') {
            height += GAME_POPUP_MESSAGE_FONT_SIZE;
        }
    }

    if (width < GAME_WINDOW_HALF_WIDTH * 0.9)
        width = GAME_WINDOW_HALF_WIDTH * 0.9;

    if (height < GAME_WINDOW_HALF_HEIGHT * 0.8)
        height = GAME_WINDOW_HALF_HEIGHT * 0.8;

    game->popup.bounds = (Rectangle) {
        .x = GAME_WINDOW_HALF_WIDTH - (width / 2.0),
        .y = GAME_WINDOW_HALF_HEIGHT - (height / 2.0),
        .width = width,
        .height = height,
    };

    game->popup.actionLeft.bounds = (Rectangle) {
        .x = game->popup.bounds.x,
        .y = game->popup.bounds.y + game->popup.bounds.height + 5,
        .width = width / 2.0,
        .height = GAME_POPUP_ACTION_TEXT_FONT_SIZE,
    };

    game->popup.actionRight.bounds = (Rectangle) {
        .x = game->popup.bounds.x + width / 2.0,
        .y = game->popup.bounds.y + game->popup.bounds.height + 5,
        .width = width / 2.0,
        .height = GAME_POPUP_ACTION_TEXT_FONT_SIZE,
    };
#undef MAX_WIDTH
}

void updateGamePopup(Game *game)
{
    Vector2 mouse = GetMousePosition();

    game->popup.actionLeft.hovering = false;
    game->popup.actionRight.hovering = false;

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, game->popup.actionLeft.bounds))
            game->popup.actionLeft.down = true;

        if (CheckCollisionPointRec(mouse, game->popup.actionRight.bounds))
            game->popup.actionRight.down = true;
    } else if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)
        && (game->popup.actionLeft.down || game->popup.actionRight.down)) {
        if (CheckCollisionPointRec(mouse, game->popup.actionLeft.bounds))
            if (game->popup.actionLeft.callback != NULL)
                game->popup.actionLeft.callback(game);

        if (CheckCollisionPointRec(mouse, game->popup.actionRight.bounds))
            if (game->popup.actionRight.callback != NULL)
                game->popup.actionRight.callback(game);

        game->popup.actionLeft.down = false;
        game->popup.actionRight.down = false;
    } else {
        if (CheckCollisionPointRec(mouse, game->popup.actionLeft.bounds))
            game->popup.actionLeft.hovering = true;

        if (CheckCollisionPointRec(mouse, game->popup.actionRight.bounds))
            game->popup.actionRight.hovering = true;
    }
}

void drawGamePopup(Game *game)
{
    char message[GAME_POPUP_MAX_MESSAGE_SIZE];
    Rectangle bounds = game->popup.bounds;
    int start = 0;

    DrawRectangleRounded(bounds, 0.3, 0, WHITE);

    bounds.y -= GAME_POPUP_TITLE_FONT_SIZE + 2.5;
    bounds.width = MeasureText(game->popup.title, GAME_POPUP_TITLE_FONT_SIZE) * 1.3;

    DrawRectangleRounded(bounds, 0.3, 0, WHITE);

    bounds.x += 5;
    bounds.y += 5;
    DrawText(game->popup.title,
        bounds.x,
        bounds.y,
        GAME_POPUP_TITLE_FONT_SIZE,
        BLUE);

    bounds.x -= 2.5;
    bounds.y += GAME_POPUP_TITLE_FONT_SIZE + 1.5;
    bounds.width = game->popup.bounds.width - 5;
    bounds.height = game->popup.bounds.height - 6.5;
    DrawRectangleRounded(bounds, 0.3, 0, BLUE);

    bounds.y += 5;
    for (int i = 0; i < strlen(game->popup.message); i++) {
        if (game->popup.message[i] != '\n')
            continue;

        game->popup.message[i] = '\0';
        DrawText(game->popup.message + start,
            bounds.x,
            bounds.y,
            GAME_POPUP_MESSAGE_FONT_SIZE,
            WHITE);
        game->popup.message[i] = '\n';

        start = i + 1;
        bounds.y += GAME_POPUP_MESSAGE_FONT_SIZE;
    }

    DrawText(game->popup.message + start,
        bounds.x,
        bounds.y,
        GAME_POPUP_MESSAGE_FONT_SIZE,
        WHITE);

    int fontSize = GAME_POPUP_ACTION_TEXT_FONT_SIZE;
    if (game->popup.actionLeft.callback != NULL) {
        bounds = game->popup.actionLeft.bounds;

        if (game->popup.actionLeft.hovering) {
            bounds.x += 1;
            bounds.y += 1;
            bounds.width -= 2;
            bounds.height -= 2;
            fontSize -= 1;
        }

        DrawRectangleRounded(bounds, 0.3, 0, WHITE);
        DrawText(game->popup.actionLeft.text,
            bounds.x + bounds.width / 2 - MeasureText(game->popup.actionLeft.text, fontSize) / 2.0,
            bounds.y,
            fontSize,
            BLUE);
    }

    fontSize = GAME_POPUP_ACTION_TEXT_FONT_SIZE;
    if (game->popup.actionRight.callback != NULL) {
        bounds = game->popup.actionRight.bounds;

        if (game->popup.actionRight.hovering) {
            bounds.x += 1;
            bounds.y += 1;
            bounds.width -= 2;
            bounds.height -= 2;
            fontSize -= 1;
        }

        DrawRectangleRounded(bounds, 0.3, 0, WHITE);
        DrawText(game->popup.actionRight.text,
            bounds.x + bounds.width / 2 - MeasureText(game->popup.actionRight.text, fontSize) / 2.0,
            bounds.y,
            fontSize,
            BLUE);
    }
}

void drawCurrentNumber(Game *game, Color color)
{
    char number[15];
    int numberWidth;

    int posX, posY;

    sprintf(number, "%d", game->values.current);
    numberWidth = MeasureText(number, GAME_FONT_SIZE);

    posX = GAME_WINDOW_HALF_WIDTH - (numberWidth / 2.0);
    posY = GAME_WINDOW_HALF_HEIGHT - (int) (GAME_FONT_SIZE * 2.5);

    DrawText(number, posX, posY, GAME_FONT_SIZE, color);
}

void drawRemainTime(Game *game, Color colors[2])
{
#define SCALAR (1.0 / 2.8)
    char number[15];
    int numberWidth;

    int posX, posY;

    sprintf(number, "%.3lf", game->time.remain);
    numberWidth = MeasureText(number, GAME_FONT_SIZE * SCALAR);

    posX = GAME_WINDOW_HALF_WIDTH - (numberWidth / 2.0);
    posY = GAME_WINDOW_HALF_HEIGHT - (int) (GAME_FONT_SIZE * 2.8);

    if (game->time.remain <= 1.5)
        DrawText(number, posX, posY, GAME_FONT_SIZE * SCALAR, colors[1]);
    else
        DrawText(number, posX, posY, GAME_FONT_SIZE * SCALAR, colors[0]);
#undef SCALAR
}

void resetGame(Game *game)
{
    // Remove generated values
    if (game->values.generated != NULL) {
        game->values.count = 0;
        free(game->values.generated);
    }

    // Generate a number to the tree root
    generateNewValue(game);

    // Destroy the tree
    if (game->treeRoot != NULL)
        destroyTree(game->treeRoot);

    // Create a new root node to start the game
    game->treeRoot = (Node *) calloc(1, sizeof(Node));

    game->treeRoot->value = game->values.current;

    game->treeRoot->right = NULL;
    game->treeRoot->left = NULL;

    game->currentTreeNode = game->treeRoot;

    // Generate the game number
    generateNewValue(game);

    // Time setup
    game->time.current = GetTime();
    game->time.remain = 15;
    game->time.start = GetTime();
}

void generateNewValue(Game *game)
{
    int i;

    // Allocate a new number entry
    game->values.generated = realloc(game->values.generated,
        sizeof(int) * ++game->values.count);

    // Generate and store
    do {
        game->values.current = RANDINT(GAME_MIN_VALUE, GAME_MAX_VALUE);

        for (i = 0; i < game->values.count; i++)
            if (game->values.current == game->values.generated[i])
                break;
    } while (i < game->values.count);

    game->values.generated[game->values.count - 1] = game->values.current;
}

void destroyTree(Node *node)
{
    if (node == NULL)
        return;

    destroyTree(node->left);
    destroyTree(node->right);

    free(node);
}

